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

#ifndef FM_RADIO_SERVICE_DBUS_CONSTANTS_H
#define FM_RADIO_SERVICE_DBUS_CONSTANTS_H

#define FM_RADIO_SERVICE_NAME "com.lomiri.FMRadioService"
#define FM_RADIO_SERVICE_OBJECT_PATH "/com/lomiri/FMRadioService"
#define FM_RADIO_SERVICE_INTERFACE "com.lomiri.FMRadioService"
#define FM_RADIO_SERVICE_TUNER_INTERFACE "com.lomiri.FMRadioService.Tuner"

#define FM_RADIO_SERVICE_ERROR_PREFIX FM_RADIO_SERVICE_INTERFACE ".Error."
#define FM_RADIO_SERVICE_ERROR_NOT_SUPPORTED "NotSupported"
#define FM_RADIO_SERVICE_ERROR_RESOURCE "ResourceError"

#define FDO_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"

#define FDO_ERROR_PREFIX "org.freedesktop.DBus.Error."
#define FDO_ERROR_FAILED "Failed"
#define FDO_ERROR_ACCESS_DENIED "AccessDenied"
#define FDO_ERROR_DISCONNECTED "Disconnected"
#define FDO_ERROR_NAME_HAS_NO_OWNER "NameHasNoOwner"
#define FDO_ERROR_SERVICE_UNKNOWN "ServiceUnknown"
#define FDO_ERROR_TIMEOUT "Timeout"
#define FDO_ERROR_UNKNOWN_OBJECT "UnknownObject"

namespace FmRadioService {
namespace DBusConstants {

enum Band {
    Unset = 0,
    AM,
    FM,
    SW,
    LW,
    FM2,
};

enum TunerState {
    Stopped = 0,
    Active,
};

} // namespace
} // namespace

#endif // FM_RADIO_SERVICE_DBUS_CONSTANTS_H
