/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "radio_control.h"

#include <QByteArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTextStream>

using namespace FmRadioService::Cherokee;

RadioControl::RadioControl(QObject *parent):
    AbstractRadioControl(parent),
    m_initializationState(InitializationState::Uninitialized),
    m_queuedFrequency(0)
{
    QString bridgePath = QStandardPaths::findExecutable("fm-bridge");
    if (bridgePath.isEmpty()) {
        QStringList androidPaths = {
            "/vendor/bin",
        };
        bridgePath = QStandardPaths::findExecutable("fm-bridge", androidPaths);
    }
    qDebug() << "fm-bridge program:" << bridgePath;
    m_bridge.setProgram(bridgePath);

    /* For some reason, on at least Halium-9 devices the LD_LIBRARY_PATH is
     * defined to point at Android's 32-bit libraries, which do not help here.
     * We also clear LD_PRELOAD, which contains some library which cannot be
     * found. */
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // The apex one is needed for libnativehelper.so
    env.insert("LD_LIBRARY_PATH", "/android/system/lib64:/android/apex/com.android.runtime/lib64");
    env.remove("LD_PRELOAD");
    m_bridge.setProcessEnvironment(env);

    QObject::connect(&m_outputObserver,
                     &audio::OutputObserver::wiredOutputIsAvailableChanged,
                     this, [this]() {
        bool isConnected = m_outputObserver.wiredOutputIsAvailable();
        qDebug() << "Antenna is connected:" << isConnected;
        updateIsAntennaConnected(isConnected);
    });

    m_playback.setRadioPortPattern("^input-fm_tuner$");
    QObject::connect(&m_playback, &audio::Playback::started,
                     this, [this]() {
        updateState(TunerState::Active);
    });
    QObject::connect(&m_playback, &audio::Playback::stopped,
                     this, [this]() {
        updateState(TunerState::Stopped);
    });
    QObject::connect(&m_playback, &audio::Playback::errorOccurred,
                     this, [this](const Error &error) {
        updateError(error);
    });
}

RadioControl::~RadioControl()
{
    if (m_initializationState == InitializationState::Initialized) {
        close();
    }
}

bool RadioControl::checkHardware()
{
    static const QStringList properties {
        QStringLiteral("vendor.bluetooth.soc"),
        QStringLiteral("vendor.qcom.bluetooth.soc"),
    };

    QProcess getprop;
    QByteArray output;
    for (const QString &property: properties) {
        getprop.start("getprop", { property });
        bool ok = getprop.waitForStarted(5000);
        if (!ok) {
            qDebug() << "'getprop' not found";
            return false;
        }
        getprop.waitForFinished(2000);
        output = getprop.readAllStandardOutput().trimmed();
        if (!output.isEmpty()) {
            qDebug() << property << "=" << output;
            break;
        }
    }
    if (output != "cherokee") {
        // not our chip
        return false;
    }

    // Try starting the bridge program
    m_bridge.start();
    bool ok = m_bridge.waitForStarted(5000);
    QByteArray errorOutput = m_bridge.readAllStandardError();
    if (!ok) {
        qWarning() << "could not start fm-bridge program:" << errorOutput;
        return false;
    }


    return true;
}

bool RadioControl::initialize()
{
    if (m_initializationState == InitializationState::Initialized ||
        m_initializationState == InitializationState::Initializing) {
        return true;
    }

    m_initializationState = InitializationState::Initializing;

    QByteArray initCommands = QByteArrayLiteral(
        "enableSlimbus 1\n"
        "setControl 0x8000004 1\n"    // HCI_FM_HELIUM_STATE
        "enableSoftMute 1\n"
        "setControl 0x8000029 0\n"    // HCI_FM_HELIUM_SET_AUDIO_PATH
        "setControl 0x800000c 1\n"    // HCI_FM_HELIUM_EMPHASIS
        "setControl 0x800000d 1\n"    // HCI_FM_HELIUM_RDS_STD
        "setControl 0x800000e 1\n"    // HCI_FM_HELIUM_SPACING
        "setControl 0x800002b 0\n"    // HCI_FM_HELIUM_SRCH_ALGORITHM
        "setControl 0x8000007 4\n"    // HCI_FM_HELIUM_REGION
        "setControl 0x8000006 0x40\n" // HCI_FM_HELIUM_RDSGROUP_MASK
        "setControl 0x8000006 0x40\n"
        "setControl 0x8000011 0\n"    // HCI_FM_HELIUM_LP_MODE
        "setControl 0x800000f 1\n"    // HCI_FM_HELIUM_RDSON
        "getControl 0x8000010\n"      // HCI_FM_HELIUM_RDSGROUP_PROC
        "setControl 0x8000010 0xef\n"
        "setControl 0x800000f 1\n"
        "setControl 0x800001b 1\n"    // HCI_FM_HELIUM_AF_JUMP
        "setControl 0x8000012 0\n"    // HCI_FM_HELIUM_ANTENNA
    );
    bool ok = sendCommands(initCommands);
    if (Q_UNLIKELY(!ok)) {
        qWarning() << "Could not initialize the bridge" <<
            m_bridge.readAllStandardError();
        raise(Error::ResourceError,
              QStringLiteral("Could not initialize the bridge"));
        return false;
    }

    m_initializationState = InitializationState::Initialized;
    return true;
}

void RadioControl::close()
{
    if (m_initializationState != InitializationState::Initialized) return;

    stop();
}

bool RadioControl::sendCommands(const QByteArray &commands)
{
    qDebug() << Q_FUNC_INFO << commands;
    const char *data = commands.constData();

    qint64 bytesRemaining = commands.size();
    qint64 bytesWritten = 0;
    while (bytesRemaining > 0) {
        int written = m_bridge.write(data + bytesWritten, bytesRemaining);
        if (written < 0) {
            qWarning() << "Could not send command" << commands;
            return false;
        }
        bytesWritten += written;
        bytesRemaining -= written;
    }

    return true;
}

bool RadioControl::waitForLine(const QByteArray &prefix, int *param1)
{
    const qint64 maxTimeout = 5000;
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < maxTimeout) {
        while (!m_bridge.canReadLine()) {
            if (timer.elapsed() >= maxTimeout) return false;
            m_bridge.waitForReadyRead(maxTimeout - timer.elapsed());
        }

        QByteArray line = m_bridge.readLine();
        qDebug() << "Read line: ***" << line;
        if (line.startsWith(prefix)) {
            QTextStream params(line.mid(prefix.length()));
            if (param1) {
                params >> *param1;
            }
            return true;
        }
    }

    return false;
}

void RadioControl::raise(Error::Code code, const QString &message)
{
    Q_EMIT errorOccurred({ code, message });
}

void RadioControl::cancelSearch()
{
}

QPair<int, int> RadioControl::frequencyRange(Band band) const
{
    Q_UNUSED(band);
    return { 0, 0 };
}

bool RadioControl::isBandSupported(Band band) const
{
    Q_UNUSED(band);
    return false;
}

int RadioControl::frequencyStep(Band band) const
{
    Q_UNUSED(band);
    return 0;
}

void RadioControl::search(int direction)
{
    updateIsSearching(true);
    QByteArray command =
        QByteArrayLiteral("startSearch ") +
        (direction > 0 ? '1' : '0') + '\n';
    sendCommands(command);
    int newFreq = 0;
    if (waitForLine("fm_seek_cmpl_cb") &&
        waitForLine("fm_tune_cb", &newFreq)) {
        updateFrequency(newFreq);
    } else {
        raise(Error::TunerError, "Scan failed");
    }
    updateIsSearching(false);
}

void RadioControl::searchBackward()
{
    if (!initialize()) return;
    search(0);
}

void RadioControl::searchForward()
{
    if (!initialize()) return;
    search(1);
}

void RadioControl::setBand(Band band)
{
    Q_UNUSED(band);
}

void RadioControl::setFrequency(int frequency)
{
    if (m_initializationState != Initialized) {
        updateFrequency(frequency);
        m_queuedFrequency = frequency;
        return;
    }

    if (frequency <= 0) {
        raise(Error::ResourceBusy,
              "Cannot reset frequency while playing");
        return;
    }

    sendCommands(QByteArrayLiteral("setFreq ") +
                 QByteArray::number(frequency) +
                 '\n');
    int newfreq;
    bool ok = waitForLine("fm_tune_cb", &newfreq);
    if (ok) {
        updateFrequency(newfreq);
    } else {
        qWarning() << "setFreq failed:" << m_bridge.readAllStandardError();
        raise(Error::TunerError, "Failed to set frequency");
    }
}

void RadioControl::setMuted(bool muted)
{
    Q_UNUSED(muted);
}

void RadioControl::setStereoMode(StereoMode mode)
{
    Q_UNUSED(mode);
}

void RadioControl::setVolume(int volume)
{
    m_playback.setVolume(volume);
    updateVolume(volume);
}

void RadioControl::start()
{
    if (initialize()) {
        if (m_queuedFrequency > 0) {
            setFrequency(m_queuedFrequency);
            m_queuedFrequency = 0;
        } else {
            /* Set some frequency, so that the user can hear that the radio is
             * active.
             */
            setFrequency(87500);
        }
        m_playback.start();
    } else {
        updateError(Error(Error::TunerError, "Cannot initialize tuner"));
    }
}

void RadioControl::stop()
{
    m_playback.stop();

    QByteArray closeCommands = QByteArrayLiteral(
        "enableSlimbus 0\n"
    );
    sendCommands(closeCommands);
    m_initializationState = Uninitialized;
}
