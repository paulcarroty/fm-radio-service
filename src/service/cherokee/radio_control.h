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

#ifndef FM_RADIO_SERVICE_CHEROKEE_RADIO_CONTROL_H
#define FM_RADIO_SERVICE_CHEROKEE_RADIO_CONTROL_H

#include "abstract_radio_control.h"

#include "audio/output_observer.h"
#include "audio/playback.h"

#include <QProcess>

namespace FmRadioService {
namespace Cherokee {

class RadioControl: public AbstractRadioControl
{
    Q_OBJECT

    enum InitializationState {
        Uninitialized = 0,
        Initializing,
        Initialized,
    };

public:
    RadioControl(QObject *parent = nullptr);
    ~RadioControl();

    bool checkHardware() override;

    void cancelSearch() override;
    QPair<int, int> frequencyRange(Band band) const override;
    bool isBandSupported(Band band) const override;
    int frequencyStep(Band band) const override;
    void searchBackward() override;
    void searchForward() override;
    void setBand(Band band) override;
    void setFrequency(int frequency) override;
    void setMuted(bool muted) override;
    void setStereoMode(StereoMode mode) override;
    void setVolume(int volume) override;
    void start() override;
    void stop() override;

private:
    bool initialize();
    void close();
    bool sendCommands(const QByteArray &commands);
    bool waitForLine(const QByteArray &prefix, int *param1 = nullptr);
    void raise(Error::Code code, const QString &message = QString());

    void search(int direction);

private:
    QProcess m_bridge;
    audio::OutputObserver m_outputObserver;
    audio::Playback m_playback;
    InitializationState m_initializationState;
    int m_queuedFrequency;
};

} // namespace
} // namespace

#endif // FM_RADIO_SERVICE_CHEROKEE_RADIO_CONTROL_H
