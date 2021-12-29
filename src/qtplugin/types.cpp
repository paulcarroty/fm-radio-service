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

#include "types.h"

#include "dbus_constants.h"

#include <QDebug>

using namespace FmRadioServicePlugin;

QRadioTuner::Band Types::bandFromDBus(quint32 value)
{
    using C = FmRadioService::DBusConstants::Band;
    switch (value) {
    case C::AM: return QRadioTuner::AM;
    case C::FM: return QRadioTuner::FM;
    case C::SW: return QRadioTuner::SW;
    case C::LW: return QRadioTuner::LW;
    case C::FM2: return QRadioTuner::FM2;
    default:
        qWarning() << "Unknown band value:" << value;
        return QRadioTuner::FM2;
    }
}

quint32 Types::bandToDBus(QRadioTuner::Band band)
{
    using C = FmRadioService::DBusConstants::Band;
    switch (band) {
    case QRadioTuner::AM: return C::AM;
    case QRadioTuner::FM: return C::FM;
    case QRadioTuner::SW: return C::SW;
    case QRadioTuner::LW: return C::LW;
    case QRadioTuner::FM2: return C::FM2;
    default:
        qWarning() << "Unknown band value:" << band;
        return C::Unset;
    }
}

QRadioTuner::State Types::stateFromDBus(quint32 value)
{
    using C = FmRadioService::DBusConstants::TunerState;
    switch (value) {
    case C::Stopped: return QRadioTuner::StoppedState;
    case C::Active: return QRadioTuner::ActiveState;
    default:
        qWarning() << "Unknown state value:" << value;
        return QRadioTuner::StoppedState;
    }
}

int Types::frequencyFromDBus(quint32 value)
{
    return value * 1000;
}

quint32 Types::frequencyToDBus(int frequency)
{
    return frequency / 1000;
}
