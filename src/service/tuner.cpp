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

#include "tuner.h"

#include "abstract_radio_control.h"
#include "radio_control_factory.h"

#include <QDebug>

using namespace FmRadioService;

namespace FmRadioService {

class TunerPrivate
{
    Q_DECLARE_PUBLIC(Tuner)

public:
    TunerPrivate(Tuner *q);

    void clearError() { m_lastError = Error(); }

private:
    QScopedPointer<AbstractRadioControl> m_control;
    Error m_lastError;
    Tuner *q_ptr;
};

} // namespace

TunerPrivate::TunerPrivate(Tuner *q):
    m_control(RadioControlFactory::createRadioControl()),
    q_ptr(q)
{
    if (m_control) {
        /*
         * Functional signals
         */
        QObject::connect(m_control.data(), &AbstractRadioControl::errorOccurred,
                         q, [this](const Error &error) {
            Q_Q(Tuner);
            m_lastError = error;
            Q_EMIT q->errorOccurred(error);
        });
        QObject::connect(m_control.data(), &AbstractRadioControl::stationFound,
                         q, &Tuner::stationFound);

        /*
         * Property change notifiers
         */
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::antennaConnectedChanged,
                         q, &Tuner::antennaConnectedChanged);
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::bandChanged,
                         q, &Tuner::bandChanged);
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::frequencyChanged,
                         q, &Tuner::frequencyChanged);
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::mutedChanged,
                         q, &Tuner::mutedChanged);
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::searchingChanged,
                         q, &Tuner::searchingChanged);
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::signalStrengthChanged,
                         q, &Tuner::signalStrengthChanged);
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::stateChanged,
                         q, &Tuner::stateChanged);
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::stereoChanged,
                         q, &Tuner::stereoChanged);
        QObject::connect(m_control.data(),
                         &AbstractRadioControl::volumeChanged,
                         q, &Tuner::volumeChanged);
    }
}

Tuner::Tuner(QObject *parent):
    QObject(parent),
    d_ptr(new TunerPrivate(this))
{
}

Tuner::~Tuner() = default;

bool Tuner::hasValidBackend() const
{
    Q_D(const Tuner);
    return d->m_control;
}

Error Tuner::lastError() const
{
    Q_D(const Tuner);
    return d->m_lastError;
}

bool Tuner::isAntennaConnected() const
{
    Q_D(const Tuner);
    return d->m_control->isAntennaConnected();
}

Band Tuner::band() const
{
    Q_D(const Tuner);
    return d->m_control->band();
}

int Tuner::frequency() const
{
    Q_D(const Tuner);
    return d->m_control->frequency();
}

int Tuner::signalStrength() const
{
    Q_D(const Tuner);
    return d->m_control->signalStrength();
}

int Tuner::volume() const
{
    Q_D(const Tuner);
    return d->m_control->volume();
}

bool Tuner::isMuted() const
{
    Q_D(const Tuner);
    return d->m_control->isMuted();
}

bool Tuner::isSearching() const
{
    Q_D(const Tuner);
    return d->m_control->isSearching();
}

bool Tuner::isStereo() const
{
    Q_D(const Tuner);
    return d->m_control->isStereo();
}

TunerState Tuner::state() const
{
    Q_D(const Tuner);
    return d->m_control->state();
}

QVariantMap Tuner::metadata() const
{
    // TODO: implement
    return QVariantMap();
}

void Tuner::setBand(Band band)
{
    Q_D(Tuner);
    d->clearError();
    d->m_control->setBand(band);
}

void Tuner::setFrequency(int frequency)
{
    Q_D(Tuner);
    d->clearError();
    d->m_control->setFrequency(frequency);
}

void Tuner::setVolume(int volume)
{
    Q_D(Tuner);
    d->clearError();
    d->m_control->setVolume(volume);
}

void Tuner::setMuted(bool muted)
{
    Q_D(Tuner);
    d->clearError();
    d->m_control->setMuted(muted);
}

void Tuner::searchForward()
{
    Q_D(Tuner);
    d->clearError();
    d->m_control->searchForward();
}

void Tuner::searchBackward()
{
    Q_D(Tuner);
    d->clearError();
    d->m_control->searchBackward();
}

void Tuner::start()
{
    Q_D(Tuner);
    d->clearError();
    d->m_control->start();
}

void Tuner::stop()
{
    Q_D(Tuner);
    d->clearError();
    d->m_control->stop();
}

void Tuner::close()
{
    stop();

    /* Reset to the initial status */
    setVolume(100);
    setFrequency(0);
    setBand(Band::Unset);

    Q_EMIT closed();
}
