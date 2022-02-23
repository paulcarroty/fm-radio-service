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

#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_ioctl.h"
#include "fmr/fmr.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <sys/ioctl.h>

using namespace FmRadioService::Mediatek;

RadioControl::RadioControl(QObject *parent):
    AbstractRadioControl(parent),
    m_initializationState(InitializationState::Uninitialized),
    m_radioDevice("/dev/fm"),
    m_audioDevice("/dev/eac"),
    m_queuedFrequency(0),
    m_fmBand(FM_BAND_UE),
    m_chipId(0),
    m_useDirectFmConnection(
        qEnvironmentVariable("FM_RADIO_SERVICE_DIRECT_AUDIO", "0") != "0")
{
    m_rdsTimer.setTimerType(Qt::VeryCoarseTimer);
    QObject::connect(&m_rdsTimer, &QTimer::timeout,
                     this, [this]() { readRds(); });
    QObject::connect(this, &AbstractRadioControl::frequencyChanged,
                     this, [this]() {
        m_rdsTimer.setInterval(1000);
        if (m_rdsTimer.isActive()) {
            m_rdsTimer.start();
        }
    });

    QObject::connect(&m_outputObserver,
                     &audio::OutputObserver::wiredOutputIsAvailableChanged,
                     this, [this]() {
        bool isConnected = m_outputObserver.wiredOutputIsAvailable();
        qDebug() << "Antenna is connected:" << isConnected;
        updateIsAntennaConnected(isConnected);
    });

    m_playback.setRadioPortPattern("^(input-fm_tuner|input-fm)$");
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
    return m_radioDevice.exists();
}

bool RadioControl::initialize()
{
    if (m_initializationState == InitializationState::Initialized ||
        m_initializationState == InitializationState::Initializing) {
        return true;
    }

    m_initializationState = InitializationState::Initializing;

    bool ok = m_radioDevice.open(QIODevice::ReadWrite);
    if (Q_UNLIKELY(!ok)) return false;

    int rc = COM_pwr_up(fd(), m_fmBand, 893);
    if (rc < 0) {
        qWarning() << "Failed to power on radio device. Please check the "
            "kernel logs to ensure that the firmware was loaded";
        return false;
    }
    getHardwareInfo();
    enableRds();
    COM_set_mute(fd(), 0);

    /* "Direct connection" means that we enable the hardware connection between
     * the FM tuner and the output device, without having to copy audio buffers
     * in userspace.
     * This avoids using the pulseaudio loopback module and the associated CPU
     * usage (from 15% to 20% in the BQ E4.5, according to `top`), but also
     * comes with a couple of shortcomings, due to the fact that pulseaudio is
     * not aware of our playback:
     *
     * 1) Audio volume cannot be controlled with the volume keys
     * 2) If a system sound gets played while the radio is on, both are
     *    audible; but after a few seconds that the system sound has stopped
     *    playing, the whole audio device will be shut off (for power-saving)
     *    and so the radio will stop playing too.
     *
     * Both of these issues should be solvable by reimplementing this audio
     * setup as a pulseaudio module. This is a big TODO (whether we get to it,
     * depends on the amount of devices which can make use of this feature).
     *
     * On some devices, like the BQ E4.5, I couldn't even find a way to get the
     * sound be played via the pulseaudio fm source port, so this is the only
     * alternative there.
     */
    if (m_chipId == 0x6627) {
        // TODO: add more chips as needed
        m_useDirectFmConnection = true;
    }

    m_initializationState = InitializationState::Initialized;
    return true;
}

void RadioControl::getHardwareInfo()
{
    struct fm_hw_info hwinfo;

	if (ioctl(fd(), FM_IOCTL_GET_HW_INFO, &hwinfo) != 0) {
		qWarning() << "FM_IOCTL_GET_HW_INFO ioctl failed";
		return;
	}

	qDebug("Chip ID: %x", hwinfo.chip_id);
	qDebug("ECO version: %x", hwinfo.eco_ver);
	qDebug("ROM Version: %x", hwinfo.rom_ver);
	qDebug("DSP patch version: %x", hwinfo.patch_ver);
    m_chipId = hwinfo.chip_id;
}

void RadioControl::enableRds()
{
    qDebug() << Q_FUNC_INFO;
    COM_turn_on_off_rds(fd(), FMR_RDS_ON);
    m_rdsTimer.setInterval(1000);
    m_rdsTimer.start();
}

void RadioControl::disableRds()
{
    qDebug() << Q_FUNC_INFO;
    COM_turn_on_off_rds(fd(), FMR_RDS_OFF);
    m_rdsTimer.stop();
}

void RadioControl::readRds()
{
    qDebug() << Q_FUNC_INFO;
    int eventsRead = 0;

    RDSData_Struct rds;

    char *buffer = (char*)&rds;
    while (m_radioDevice.peek(buffer, sizeof(rds)) == sizeof(rds)) {
        qint64 bytesRead = m_radioDevice.read(buffer, sizeof(rds));
        if (bytesRead < qint64(sizeof(rds))) {
            qWarning() << "Error reading RDS data";
            break;
        }

        eventsRead++;
        if (rds.event_status & (RDS_EVENT_UTCDATETIME | RDS_EVENT_LOCDATETIME)) {
            qDebug() << "month" << rds.CT.Month;
            qDebug() << "day" <<   rds.CT.Day;
            qDebug() << "year"  << rds.CT.Year;
            qDebug() << "hour"  << rds.CT.Hour;
            qDebug() << "min"   << rds.CT.Minute;
            qDebug() << "ofset" << rds.CT.Local_Time_offset_signbit;
            qDebug() << "half"  << rds.CT.Local_Time_offset_half_hour;
        }

        if (rds.event_status & RDS_EVENT_FLAGS) {
            qDebug() << "TP"    << rds.RDSFlag.TP;
            qDebug() << "TA"    << rds.RDSFlag.TA;
            qDebug() << "Music" << rds.RDSFlag.Music;
            qDebug() << "Stere" << rds.RDSFlag.Stereo;
            qDebug() << "ahead" << rds.RDSFlag.Artificial_Head;
            qDebug() << "comp"  << rds.RDSFlag.Compressed;
            qDebug() << "dynpt" << rds.RDSFlag.Dynamic_PTY;
            qDebug() << "texta" << rds.RDSFlag.Text_AB;
            qDebug() << "flag"  << hex << rds.RDSFlag.flag_status;
        }

        if (rds.event_status & RDS_EVENT_PI_CODE) {
            qDebug() << "PI   " << hex << rds.PI;
        }

        if (rds.event_status & RDS_EVENT_PROGRAMNAME) {
            for (int i = 0; i < 4; i++) {
                qDebug() << "PS" << QByteArray::fromRawData((const char *)rds.PS_Data.PS[i], 8);
            }
            qDebug() << "addrcnt" << rds.PS_Data.Addr_Cnt;
        }

        if (rds.event_status & RDS_EVENT_LAST_RADIOTEXT) {
            for (int i = 0; i < 4; i++) {
                qDebug() << "RT" << QByteArray::fromRawData((const char *)rds.RT_Data.TextData[i], 64);
            }
            qDebug() << "getlength" << rds.RT_Data.GetLength;
            qDebug() << "isRTdisplay" << rds.RT_Data.isRTDisplay;
            qDebug() << "textlength" << rds.RT_Data.TextLength;
            qDebug() << "istypea" << rds.RT_Data.isTypeA;
            qDebug() << "bufcnt" << rds.RT_Data.BufCnt;
            qDebug() << "addrcnt" << rds.RT_Data.Addr_Cnt;
        }

        qDebug() << "event status" << hex << rds.event_status;
    }
    qDebug() << "Events read:" << eventsRead;
    qDebug() << "could read more bytes:" << m_radioDevice.peek(buffer, sizeof(rds));

    // Decide whether this timer should be executed more or less frequently
    int oldInterval = m_rdsTimer.interval();
    int newInterval;
    if (eventsRead > 1) {
        newInterval = qMax(oldInterval / eventsRead, 1000);

    } else if (eventsRead == 0) {
        newInterval = qMin(int(oldInterval * 1.5), 5 * 60 * 1000);
    } else {
        newInterval = oldInterval;
    }

    if (newInterval != oldInterval) {
        qDebug() << "Adjusting interval to" << newInterval;
        m_rdsTimer.setInterval(newInterval);
        m_rdsTimer.start();
    }
}

bool RadioControl::initAudio()
{
    if (!m_audioDevice.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open audio device /dev/eac";
        // returning "true" because it may be that the absence of this file
        // means that no audio setup is needed.
        return true;
    }

    int audioFd = m_audioDevice.handle();
    int ret = 0;

#define FIRST_SETUP
#ifdef FIRST_SETUP
    // AudioResourceManager EnableAudioClock AudioClockType = 1 bEnable = 1
    ret = ioctl(audioFd, AUD_SET_CLOCK, 1);
    qDebug() << "ret =" << ret;

    // AudioResourceManager EnableAudioClock AudioClockType = 4 bEnable = 1
    ret = ioctl(audioFd, AUD_SET_ANA_CLOCK, 1);
    qDebug() << "ret =" << ret;
#endif

    qDebug() << "AFE setup";

    /* Registers and enums are in
     * mediatek/platform/mt6582/kernel/drivers/sound/AudDrv_Afe.h
     */

    setAfeReg(AFE_DAC_CON0, 0x1, 0x1);
    /* we don't really know what the factory mode is calling here, but the
     * choice is between long (0) and short (1) */
    uint32_t useShortAntenna = 0;
    ioctl(fd(), FM_IOCTL_ANA_SWITCH, &useShortAntenna);

    qDebug() << "Activating digital FM input";
    ret = ioctl(audioFd, AUDDRV_SET_FM_I2S_GPIO);
    qDebug() << "ret =" << ret;

    {
        // AudioFMResourceManager::SetFmSourceModuleEnable
        // +SetFmSourceModuleEnable(), enable = 1

        // AudioDigitalControl Set2ndI2SIn Audio_I2S_Adc= 0x8000000c
        setAfeReg(AFE_I2S_CON, 0x8000000c, 0xfffffffe);
        setAfeReg(FPGA_CFG1, 0x0, 0x100);
        {
            // AudioDigitalControl::SetI2SASRCConfig
            // +SetI2SASRCConfig() bIsUseASRC [1] dToSampleRate [44100]
            setAfeReg(AFE_CONN4, 0x0, 0x40000000);
            // +SampleRateTransform(), SampleRate = 44100
            // +SetMemIfSampleRate(), InterfaceType = 4, SampleRate = 44100, mAudioMEMIF[InterfaceType].mSampleRate = 9
            setAfeReg(AFE_DAC_CON1, 0x900, 0xf00);
            setAfeReg(AFE_ASRC_CON13, 0x0, 0x10000);
            setAfeReg(AFE_ASRC_CON14, 0xdc8000, 0xffffffff);
            setAfeReg(AFE_ASRC_CON15, 0xa00000, 0xffffffff);
            setAfeReg(AFE_ASRC_CON17, 0x1fbd, 0xffffffff);
            setAfeReg(AFE_ASRC_CON16, 0x75987, 0xffffffff);
            setAfeReg(AFE_ASRC_CON20, 0x1b00, 0xffffffff);
        }

        // AudioDigitalControl::SetI2SASRCEnable
        setAfeReg(AFE_ASRC_CON0, 0x41, 0x41);
        // -SetFmSourceModuleEnable(), enable = 1
    }

    {
        // AudioDigitalControl Set2ndI2SInEnable bEnable = 1
        setAfeReg(AFE_I2S_CON, 0x1, 0x1);
    }

    // AudioFMController +SetFmDirectConnection(), enable = 1, bforce = 1
    // AudioFMResourceManager +SetFmDirectConnection(), enable = 1

    // +SetFmDirectConnection(), enable = 1
    /* The connection registers are looked up the mConnectionReg 2-D table in
     * mediatek/platform/mt6582/hardware/audio/aud_drv/AudioInterConnection.cpp
     */
    // SetinputConnection(), ConnectionState = 1, Input = 0, Output = 15
    setAfeReg(AFE_GAIN2_CONN, 0x10000, 0x10000);
    // SetinputConnection(), ConnectionState = 1, Input = 0, Output = 16
    setAfeReg(AFE_GAIN2_CONN, 0x800000, 0x800000);
    // SetinputConnection ConnectionState = 1 Input = 12 Output = 3
    setAfeReg(AFE_GAIN2_CONN, 0x100, 0x100);
    // SetinputConnection(), ConnectionState = 1, Input = 13, Output = 4
    setAfeReg(AFE_GAIN2_CONN, 0x400, 0x400);

    // SampleRateTransform(), SampleRate = 44100
    // SetHwDigitalGainMode(), GainType = 1, SampleRate = 9, SamplePerStep= 64
    setAfeReg(AFE_GAIN2_CON0, 0x4090, 0xfff0);

    // SetFmVolume(), fm_volume = 0.000000
    // SetHwDigitalGain(), Gain = 0x0, gain type = 1
    setAfeReg(AFE_GAIN2_CON1, 0x0, 0xffffffff);

    // SetFmVolume(), fm_volume = 0.000000
    {
        // SetHwDigitalGainEnable(), GainType = 1, Enable = 1
        setAfeReg(AFE_GAIN2_CUR, 0, 0xffffffff);
        setAfeReg(AFE_GAIN2_CON0, 0x1, 0x1);
    }

    // SetI2SDacOutAttribute(), sampleRate = 44100
    {
        // AudioDigitalControl::SetI2SDacOut
        setAfeReg(AFE_PREDIS_CON0, 0x0, 0xffffffff);
        setAfeReg(AFE_PREDIS_CON1, 0x0, 0xffffffff);
        // SetI2SDacOut(), AfeAddaDLSrc2Con0[0x108] = 0x73001802
        setAfeReg(AFE_ADDA_DL_SRC2_CON0, 0x73001802, 0xffffffff);
        // SetI2SDacOut(), AfeAddaDLSrc2Con1[0x10c] = 0xf74f0000
        setAfeReg(AFE_ADDA_DL_SRC2_CON1, 0xf74f0000, 0xffffffff);
        // SetI2SDacOut(), Audio_I2S_Dac[0x34] = 0x908
        setAfeReg(AFE_I2S_CON1, 0x908, 0xffffffff);
    }

    {
        // SetI2SDacEnable(), bEnable = 1
        setAfeReg(AFE_ADDA_DL_SRC2_CON0, 0x1, 0x1);
        setAfeReg(AFE_I2S_CON1, 0x1, 0x1);
        setAfeReg(AFE_ADDA_UL_DL_CON0, 0x1, 0x1);
        setAfeReg(FPGA_CFG1, 0x0, 0x10);
    }

    // AudioFMResourceManager -SetFmDirectConnection(), enable = 1

    // +SetHwDigitalGain(), Gain = 0x0, gain type = 1
    setAfeReg(AFE_GAIN2_CON1, 0x0, 0xffffffff);
    // AudioFMController -SetFmDirectConnection(), enable = 1

    // AudioPlatformDevice -->
    {
        // AudioPlatformDevice::AnalogOpen
        // AudioPlatformDevice AnalogOpen DeviceType = DEVICE_OUT_HEADSETR
        setAnalogReg(TOP_CKPDN1_CLR, 0x100, 0x100);
        setAnalogReg(ABB_AFE_PMIC_NEWIF_CFG0, 0x7330, 0xffff);
        setAnalogReg(ABB_AFE_CON1, 0x9, 0x000f);
        // AnalogOpen mHpRightDcCalibration [0x0] mHpLeftDcCalibration [0x0]
        // but I've seen values of 0x289 and 0x298! TODO FIXME diff!
        setAnalogReg(ABB_AFE_CON3, 0x284, 0xffff);
        setAnalogReg(ABB_AFE_CON4, 0x28f, 0xffff);
        setAnalogReg(ABB_AFE_CON10, 0x1, 0x1); // enable DC compensation
        // Now it checks for some calibration data:
        ret = getAnalogReg(ABB_AFE_CON11);
        qDebug() << "ret =" << ret;

        setAnalogReg(ABB_AFE_CON11, 0x200, 0x200);
        ret = getAnalogReg(ABB_AFE_CON11);
        qDebug() << "ret =" << ret << "(expected 202)";

        setAnalogReg(ABB_AFE_CON11, 0x100, 0x100);
        // now back in AudioPlatformDevice::AnalogOpen
        setAnalogReg(ABB_AFE_CON0, 0x1, 0x1); // turn on DL
    }

    {
        // AudioMachineDevice AnalogOpen DeviceType = DEVICE_OUT_HEADSETR
        setAnalogReg(AUDTOP_CON6, 0xf7f2, 0xffff);
        setAnalogReg(AUDTOP_CON0, 0x7000, 0xf000);
        setAnalogReg(AUDTOP_CON5, 0x0014, 0xffff);
        setAnalogReg(AUDTOP_CON4, 0x007c, 0xffff);
        QThread::usleep(10000);
        setAnalogReg(AUDTOP_CON6, 0xf5ba, 0xffff);
        setAnalogReg(AUDTOP_CON5, 0x2214, 0xffff);
        ioctl(audioFd, SET_HEADPHONE_ON, 0);
    }


    // AudioMTKVolumeController setMasterVolume v = 1.000000 mode = 0 devices = 0x8
    // ApplyAudioGain  Gain = 256 mode= 0 device = 2
    // ApplyAudioGain  DegradedBGain = 38 mVolumeRange[mode] = 38
    // AudioMachineDevice SetAnalogGain VOLUME_TYPE = 2 volume = 38
    setAnalogReg(AUDTOP_CON5, 0x7, 0x7000);
    // AudioMachineDevice SetAnalogGain VOLUME_TYPE = 3 volume = 38
    setAnalogReg(AUDTOP_CON5, 0x7, 0x0700);

    // AudioFMController -SetFmEnable()
    // AudioFMController +SetFmVolume(), mFmVolume = 0.000000 => fm_volume = 1.000000
    // AudioDigitalControl +SetHwDigitalGain(), Gain = 0x80000, gain type = 1
    setAfeReg(AFE_GAIN2_CON1, 0x80000, 0xffffffff);
    // -SetFmVolume(), mFmVolume = 1.000000

    qDebug() << "getting debug info:";
    ret = ioctl(audioFd, AUDDRV_LOG_PRINT);

    return true;
}

void RadioControl::closeAudio() {
    m_audioDevice.close();
}

int RadioControl::setAfeReg(int reg, int value, int mask)
{
    Register_Control rc;
    rc.offset = reg;
    rc.value = value;
    rc.mask = mask;
    return ioctl(m_audioDevice.handle(), SET_AUDSYS_REG, &rc);
}

int RadioControl::setAnalogReg(int reg, int value, int mask)
{
    Register_Control rc;
    rc.offset = reg;
    rc.value = value;
    rc.mask = mask | 0xffff0000;
    return ioctl(m_audioDevice.handle(), SET_ANAAFE_REG, &rc);
}

uint32_t RadioControl::getAnalogReg(int reg)
{
    Register_Control rc;
    rc.offset = reg;
    rc.value = 0;
    rc.mask = 0xffffffff;
    ioctl(m_audioDevice.handle(), GET_ANAAFE_REG, &rc);
    return rc.value;
}

int RadioControl::fd() const
{
    return m_radioDevice.handle();
}

void RadioControl::close()
{
    if (m_initializationState != InitializationState::Initialized) return;

    stop();
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

void RadioControl::manualSearch(int currentFreq, int direction)
{
    const int freqMin = FMR_BAND_FREQ_L;
    const int freqMax = FMR_BAND_FREQ_H;

    int inc = direction == 1 ? 1 : -1;
    currentFreq += inc;
    if (currentFreq > freqMax) {
        currentFreq = freqMin;
    } else if (currentFreq < freqMin) {
        currentFreq = freqMax;
    }

    /* Check if we looped over */
    int startingFrequency = frequency() / 100;
    if (currentFreq == startingFrequency) {
        updateIsSearching(false);
        return;
    }

    bool failed = false;
    int ret = COM_tune(fd(), currentFreq, m_fmBand);
    if (ret != 0) {
        qWarning() << "Error tuning during search";
        failed = true;
    } else {
        int rssi = 0;
        ret = ioctl(fd(), FM_IOCTL_GETRSSI, &rssi);
        if (ret) {
            qWarning() << "Error getting RSSI";
            failed = true;
        } else {
            qDebug() << "RSSI" << rssi << "on frequency" << currentFreq;
            if (rssi > -65) {
                updateFrequency(currentFreq * 100);
                updateIsSearching(false);
            } else {
                QMetaObject::invokeMethod(this, [this, currentFreq, direction]() {
                    manualSearch(currentFreq, direction);
                }, Qt::QueuedConnection);
            }
        }
    }

    if (failed) {
        updateIsSearching(false);
        // Reset to the previous frequency
        COM_tune(fd(), frequency() / 100, m_fmBand);
    }
}

void RadioControl::search(int direction)
{
    updateIsSearching(true);
    int freq = frequency() / 100;
    int ret = COM_seek(fd(), &freq, m_fmBand, direction,
                      FM_SEEKTH_LEVEL_DEFAULT);
    if (ret == 0) {
        qDebug() << "Frequency after seek:" << freq;
        updateFrequency(freq * 100);
        updateIsSearching(false);
    } else {
        qWarning() << "Seek failed, starting manual search";
        manualSearch(freq, direction);
    }
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

    COM_tune(fd(), frequency / 100, m_fmBand);
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
    // max volume is 15
    int fmVolume = (volume * 15) / 100;
    if (ioctl(fd(), FM_IOCTL_SETVOL, &fmVolume) != 0)
	{
		qWarning("FM_IOCTL_SETVOL, ioctl failed");
	}

    //m_playback.setVolume(volume);
}

void RadioControl::start()
{
    if (initialize()) {
        if (m_queuedFrequency > 0) {
            setFrequency(m_queuedFrequency);
            m_queuedFrequency = 0;
        }
        qDebug() << "Using direct FM connection:" << m_useDirectFmConnection;
        if (m_useDirectFmConnection) {
            initAudio();
        } else {
            m_playback.start();
        }
    } else {
        updateError(Error(Error::TunerError, "Cannot initialize tuner"));
    }
}

void RadioControl::stop()
{
    if (m_useDirectFmConnection) {
        closeAudio();
    } else {
        m_playback.stop();
    }

    disableRds();
    COM_pwr_down(fd(), FM_RX);
    m_radioDevice.close();
    m_initializationState = Uninitialized;
}
