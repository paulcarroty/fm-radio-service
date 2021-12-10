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

#include "abstract_radio_control.h"

using namespace FmRadioService;

namespace FmRadioService {

class AbstractRadioControlPrivate
{
    Q_DECLARE_PUBLIC(AbstractRadioControl)

public:
    AbstractRadioControlPrivate(AbstractRadioControl *q);

private:
    Band m_band = Band::Unset;
    int m_frequency = 0;
    Error m_error;
    /* QtMultimedia's QRadioTuner will return an error if the antenna is not
     * connected when it gets contructed. So, let's start up in taht state;
     * the real information will be updated later by the backend.
     */
    bool m_isAntennaConnected = true;
    bool m_isMuted = false;
    bool m_isSearching = false;
    bool m_isStereo = false;
    int m_signalStrength = 0;
    TunerState m_state = TunerState::Stopped;
    StereoMode m_stereoMode = StereoMode::Auto;
    int m_volume = 100;
    AbstractRadioControl *q_ptr;
};

} // namespace

AbstractRadioControlPrivate::AbstractRadioControlPrivate(
        AbstractRadioControl *q):
    q_ptr(q)
{
}

AbstractRadioControl::AbstractRadioControl(QObject *parent):
    QObject(parent),
    d_ptr(new AbstractRadioControlPrivate(this))
{
}

AbstractRadioControl::~AbstractRadioControl() = default;

void AbstractRadioControl::updateBand(Band band)
{
    Q_D(AbstractRadioControl);
    if (band != d->m_band) {
        d->m_band = band;
        Q_EMIT bandChanged();
    }
}

Band AbstractRadioControl::band() const
{
    Q_D(const AbstractRadioControl);
    return d->m_band;
}

void AbstractRadioControl::updateFrequency(int frequency)
{
    Q_D(AbstractRadioControl);
    if (frequency != d->m_frequency) {
        d->m_frequency = frequency;
        Q_EMIT frequencyChanged();
    }
}

int AbstractRadioControl::frequency() const
{
    Q_D(const AbstractRadioControl);
    return d->m_frequency;
}

void AbstractRadioControl::updateError(const Error &error)
{
    Q_D(AbstractRadioControl);
    if (error != d->m_error) {
        d->m_error = error;
        Q_EMIT errorOccurred(error);
    }
}

Error AbstractRadioControl::error() const
{
    Q_D(const AbstractRadioControl);
    return d->m_error;
}

void AbstractRadioControl::updateIsAntennaConnected(bool isAntennaConnected)
{
    Q_D(AbstractRadioControl);
    if (isAntennaConnected != d->m_isAntennaConnected) {
        d->m_isAntennaConnected = isAntennaConnected;
        Q_EMIT antennaConnectedChanged();
    }
}

bool AbstractRadioControl::isAntennaConnected() const
{
    Q_D(const AbstractRadioControl);
    return d->m_isAntennaConnected;
}

void AbstractRadioControl::updateIsMuted(bool isMuted)
{
    Q_D(AbstractRadioControl);
    if (isMuted != d->m_isMuted) {
        d->m_isMuted = isMuted;
        Q_EMIT mutedChanged();
    }
}

bool AbstractRadioControl::isMuted() const
{
    Q_D(const AbstractRadioControl);
    return d->m_isMuted;
}

void AbstractRadioControl::updateIsSearching(bool isSearching)
{
    Q_D(AbstractRadioControl);
    if (isSearching != d->m_isSearching) {
        d->m_isSearching = isSearching;
        Q_EMIT searchingChanged();
    }
}

bool AbstractRadioControl::isSearching() const
{
    Q_D(const AbstractRadioControl);
    return d->m_isSearching;
}

void AbstractRadioControl::updateIsStereo(bool isStereo)
{
    Q_D(AbstractRadioControl);
    if (isStereo != d->m_isStereo) {
        d->m_isStereo = isStereo;
        Q_EMIT stereoChanged();
    }
}

bool AbstractRadioControl::isStereo() const
{
    Q_D(const AbstractRadioControl);
    return d->m_isStereo;
}

void AbstractRadioControl::updateSignalStrength(int signalStrength)
{
    Q_D(AbstractRadioControl);
    if (signalStrength != d->m_signalStrength) {
        d->m_signalStrength = signalStrength;
        Q_EMIT signalStrengthChanged();
    }
}

int AbstractRadioControl::signalStrength() const
{
    Q_D(const AbstractRadioControl);
    return d->m_signalStrength;
}

void AbstractRadioControl::updateState(TunerState state)
{
    Q_D(AbstractRadioControl);
    if (state != d->m_state) {
        d->m_state = state;
        Q_EMIT stateChanged();
    }
}

TunerState AbstractRadioControl::state() const
{
    Q_D(const AbstractRadioControl);
    return d->m_state;
}

void AbstractRadioControl::updateStereoMode(StereoMode stereoMode)
{
    Q_D(AbstractRadioControl);
    if (stereoMode != d->m_stereoMode) {
        d->m_stereoMode = stereoMode;
    }
}

StereoMode AbstractRadioControl::stereoMode() const
{
    Q_D(const AbstractRadioControl);
    return d->m_stereoMode;
}

void AbstractRadioControl::updateVolume(int volume)
{
    Q_D(AbstractRadioControl);
    if (volume != d->m_volume) {
        d->m_volume = volume;
        Q_EMIT volumeChanged();
    }
}

int AbstractRadioControl::volume() const
{
    Q_D(const AbstractRadioControl);
    return d->m_volume;
}
