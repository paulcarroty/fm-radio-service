/*
 * Copyright © 2021 UBports Foundation.
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

#include "dbus_error.h"

#include "dbus_constants.h"

#include <QDebug>
#include <QString>

using namespace FmRadioService;

QString DBusError::code(const Error &error)
{
    QString code;
    switch (error.code()) {
    case Error::ResourceError:
        code = FM_RADIO_SERVICE_ERROR_PREFIX FM_RADIO_SERVICE_ERROR_RESOURCE;
        break;
    default:
        qWarning() << "Unknown error code" << error.code();
        code = FDO_ERROR_PREFIX FDO_ERROR_FAILED;
    }
    return code;
}
