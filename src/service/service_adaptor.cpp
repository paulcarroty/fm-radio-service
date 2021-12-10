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

#include "service_adaptor.h"

#include "dbus_error.h"
#include "service.h"
#include "tuner_adaptor.h"

#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusServiceWatcher>

using namespace FmRadioService;

namespace FmRadioService {

class ServiceAdaptorPrivate
{
    Q_DECLARE_PUBLIC(ServiceAdaptor)

public:
    ServiceAdaptorPrivate(Service *service, ServiceAdaptor *q);

    void registerClient(const QString &clientId);
    void unregisterClient(const QString &clientId);

private:
    Service *m_service;
    QDBusServiceWatcher m_serviceWatcher;
    int m_nextAdaptorId = 0;
    ServiceAdaptor *q_ptr;
};

} // namespace

ServiceAdaptorPrivate::ServiceAdaptorPrivate(
        Service *service,
        ServiceAdaptor *q):
    m_service(service),
    q_ptr(q)
{
    m_serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    QObject::connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
                     q, [this](const QString &clientId) {
        qDebug() << "Client left:" << clientId;
        unregisterClient(clientId);
    });
}

void ServiceAdaptorPrivate::registerClient(const QString &clientId)
{
    Q_Q(ServiceAdaptor);
    m_serviceWatcher.addWatchedService(clientId);
    /* In case the client made the call and immediately quit (for example, if
     * it was a `gdbus call`), we'll find that it has already quit; if that's
     * the case, don't keep the session open.
     */
    QDBusReply<uint> pidReply =
        q->connection().interface()->servicePid(clientId);
    if (pidReply.isValid()) {
        qDebug() << "Registered new client" << clientId <<
            "with PID" << pidReply.value();
    } else {
        qDebug() << "Received a message from a client which already quit:" <<
            clientId;
        unregisterClient(clientId);
    }
}

void ServiceAdaptorPrivate::unregisterClient(const QString &clientId)
{
    Q_Q(ServiceAdaptor);
    m_serviceWatcher.removeWatchedService(clientId);
    /* Check if this client was controlling a Tuner and, if so, close the
     * tuner */
    const QList<TunerAdaptor*> adaptors = q->findChildren<TunerAdaptor*>();
    for (TunerAdaptor *tunerAdaptor: adaptors) {
        qDebug() << "adaptor used by client" << tunerAdaptor->clientId();
        if (tunerAdaptor->clientId() == clientId) {
            tunerAdaptor->Close();
        }
    }
}

ServiceAdaptor::ServiceAdaptor(QObject *parent):
    QObject(parent),
    d_ptr(new ServiceAdaptorPrivate(qobject_cast<Service*>(parent), this))
{
}

ServiceAdaptor::~ServiceAdaptor() = default;

bool ServiceAdaptor::registerAt(QDBusConnection &connection,
                                const QString &objectPath)
{
    Q_D(ServiceAdaptor);
    d->m_serviceWatcher.setConnection(connection);
    return connection.registerObject(
            objectPath,
            this,
            QDBusConnection::ExportAllSlots |
            QDBusConnection::ExportScriptableSignals |
            QDBusConnection::ExportAllProperties);
}

Service *ServiceAdaptor::service()
{
    Q_D(ServiceAdaptor);
    return d->m_service;
}

const Service *ServiceAdaptor::service() const
{
    Q_D(const ServiceAdaptor);
    return d->m_service;
}

QDBusObjectPath ServiceAdaptor::OpenTuner()
{
    Q_D(ServiceAdaptor);
    Tuner *tuner = service()->openTuner();
    if (tuner) {
        QString clientId = message().service();
        d->registerClient(clientId);
        int uniqueId = d->m_nextAdaptorId++;
        QString objectPath = QString("/Session/%1").arg(uniqueId);
        TunerAdaptor *tunerAdaptor = new TunerAdaptor(tuner, clientId, this);
        QDBusConnection conn = connection();
        tunerAdaptor->registerAt(conn, objectPath);
        return QDBusObjectPath(objectPath);
    } else {
        Error error = service()->lastError();
        QString code = DBusError::code(error);
        sendErrorReply(code, error.message());
        return QDBusObjectPath();
    }
}
