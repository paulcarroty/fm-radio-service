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

#ifndef FM_RADIO_SERVICE_MEDIATEK_RADIO_CONTROL_H
#define FM_RADIO_SERVICE_MEDIATEK_RADIO_CONTROL_H

#include "abstract_radio_control.h"

#include "audio/output_observer.h"
#include "audio/playback.h"

#include <QFile>
#include <QTimer>

namespace FmRadioService {
namespace Mediatek {

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
    void getHardwareInfo();
    int fd() const;
    void close();
    void raise(Error::Code code, const QString &message = QString());

    void enableRds();
    void disableRds();
    void readRds();

    void manualSearch(int currentFreq, int direction);
    void search(int direction);

    bool initAudio();
    void closeAudio();
    int setAfeReg(int reg, int value, int mask);
    int setAnalogReg(int reg, int value, int mask);
    uint32_t getAnalogReg(int reg);

private:
    audio::OutputObserver m_outputObserver;
    audio::Playback m_playback;
    InitializationState m_initializationState;
    QFile m_radioDevice;
    QFile m_audioDevice;
    int m_queuedFrequency;
    int m_fmBand;
    int m_chipId;
    bool m_useDirectFmConnection;
    QTimer m_rdsTimer;
};

} // namespace
} // namespace

#endif // FM_RADIO_SERVICE_MEDIATEK_RADIO_CONTROL_H
