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

#include "error.h"

#include "dbus_constants.h"

#include <QDBusMessage>
#include <QDebug>

using namespace FmRadioServicePlugin;

Error Error::fromDBus(const QDBusMessage &msg)
{
    QRadioTuner::Error code = QRadioTuner::NoError;

    const QString dbusCode = msg.errorName();
    if (dbusCode.startsWith(QStringLiteral(FM_RADIO_SERVICE_ERROR_PREFIX))) {
        const size_t prefixLength = sizeof(FM_RADIO_SERVICE_ERROR_PREFIX) - 1;
        const QStringRef name = dbusCode.midRef(prefixLength);
        if (name == FM_RADIO_SERVICE_ERROR_NOT_SUPPORTED) {
            code = QRadioTuner::OpenError;
        } else if (name == FM_RADIO_SERVICE_ERROR_RESOURCE) {
            code = QRadioTuner::ResourceError;
        }
    } else if (dbusCode.startsWith(QStringLiteral(FDO_ERROR_PREFIX))) {
        const size_t prefixLength = sizeof(FDO_ERROR_PREFIX) - 1;
        const QStringRef name = dbusCode.midRef(prefixLength);
        if (name == FDO_ERROR_ACCESS_DENIED) {
            code = QRadioTuner::ResourceError;
        } else if (name == FDO_ERROR_DISCONNECTED ||
                   name == FDO_ERROR_NAME_HAS_NO_OWNER ||
                   name == FDO_ERROR_SERVICE_UNKNOWN) {
            code = QRadioTuner::OpenError;
        }
    }

    if (code == QRadioTuner::NoError) {
        qWarning() << "Unhandled error code" << dbusCode;
        code = QRadioTuner::ResourceError;
    }

    return Error(code, msg.errorMessage());
}
