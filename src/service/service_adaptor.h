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

#ifndef FM_RADIO_SERVICE_SERVICE_ADAPTOR_H
#define FM_RADIO_SERVICE_SERVICE_ADAPTOR_H

#include "dbus_constants.h"

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QString>

namespace FmRadioService
{

class Service;

class ServiceAdaptorPrivate;
class ServiceAdaptor: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", FM_RADIO_SERVICE_INTERFACE)

public:
    ServiceAdaptor(QObject *parent = nullptr);
    ~ServiceAdaptor();

    bool registerAt(QDBusConnection &connection, const QString &objectPath);

    Service *service();
    const Service *service() const;

public Q_SLOTS:
    QDBusObjectPath OpenTuner();

private:
    Q_DECLARE_PRIVATE(ServiceAdaptor)
    QScopedPointer<ServiceAdaptorPrivate> d_ptr;
};

} // namespace

#endif // FM_RADIO_SERVICE_SERVICE_ADAPTOR_H
