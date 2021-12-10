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

#ifndef FM_RADIO_SERVICE_ABSTRACT_RADIO_CONTROL_H
#define FM_RADIO_SERVICE_ABSTRACT_RADIO_CONTROL_H

#include "error.h"
#include "types.h"

#include <QPair>
#include <QString>

namespace FmRadioService {

/* This file is heavily based on QRadioTunerControl. We don't directly use it,
 * though, because it's been removed in Qt6 and we don't want to make the
 * future transition any harder than what it will be.
 */

class AbstractRadioControlPrivate;
class AbstractRadioControl: public QObject
{
    Q_OBJECT

public:
    AbstractRadioControl(QObject *parent = nullptr);
    ~AbstractRadioControl();

    /* This should return true only if this backend is compatible with the
     * current device. */
    virtual bool checkHardware() = 0;

    Band band() const;
    int frequency() const;
    Error error() const;
    bool isAntennaConnected() const;
    bool isMuted() const;
    bool isSearching() const;
    bool isStereo() const;
    int signalStrength() const;
    TunerState state() const;
    StereoMode stereoMode() const;
    int volume() const;

    virtual void cancelSearch() = 0;
    virtual QPair<int, int> frequencyRange(Band band) const = 0;
    virtual bool isBandSupported(Band band) const = 0;
    virtual int frequencyStep(Band band) const = 0;
    virtual void searchBackward() = 0;
    virtual void searchForward() = 0;
    virtual void setBand(Band band) = 0;
    virtual void setFrequency(int frequency) = 0;
    virtual void setMuted(bool muted) = 0;
    virtual void setStereoMode(StereoMode mode) = 0;
    virtual void setVolume(int volume) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

Q_SIGNALS:
    void errorOccurred(const Error &error);
    void stationFound(int frequency, QString stationId);

    void antennaConnectedChanged();
    void bandChanged();
    void frequencyChanged();
    void mutedChanged();
    void searchingChanged();
    void signalStrengthChanged();
    void stateChanged();
    void stereoChanged();
    void volumeChanged();

protected:
    void updateBand(Band band);
    void updateFrequency(int frequency);
    void updateError(const Error &error);
    void updateIsAntennaConnected(bool isConnected);
    void updateIsMuted(bool isMuted);
    void updateIsSearching(bool isSearching);
    void updateIsStereo(bool isStereo);
    void updateSignalStrength(int strength);
    void updateState(TunerState state);
    void updateStereoMode(StereoMode mode);
    void updateVolume(int volume);

private:
    Q_DECLARE_PRIVATE(AbstractRadioControl)
    QScopedPointer<AbstractRadioControlPrivate> d_ptr;
};

} // namespace

#endif // FM_RADIO_SERVICE_ABSTRACT_RADIO_CONTROL_H
