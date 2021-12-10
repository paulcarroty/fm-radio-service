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

#include "dbus_constants.h"
#include "inactivity_timer.h"
#include "service.h"
#include "service_adaptor.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QProcessEnvironment>
#include <QScopedPointer>
#include <signal.h>

static void signalHandler(int signum) {
    if (signum == SIGTERM ||
        signum == SIGINT) {
        QCoreApplication::quit();
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    qDebug() << "fm-radio service started";

    {
        /* Cleanly quit on SIGTERM */
        struct sigaction sa;
        sa.sa_flags = 0;
        sa.sa_handler = signalHandler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGTERM, &sa, 0);
        sigaction(SIGINT, &sa, 0);
    }

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();

    /* default daemonTimeout to 5 seconds */
    int daemonTimeout = 5;

    if (environment.contains(QLatin1String("FM_RADIO_SERVICE_TIMEOUT"))) {
        bool isOk;
        int value = environment.value(
            QLatin1String("FM_RADIO_SERVICE_TIMEOUT")).toInt(&isOk);
        if (isOk)
            daemonTimeout = value;
    }

    QScopedPointer<FmRadioService::Service> service(
        new FmRadioService::Service());

    QScopedPointer<FmRadioService::ServiceAdaptor> serviceAdaptor(
        new FmRadioService::ServiceAdaptor(service.data()));

    FmRadioService::InactivityTimer inactivityTimer(daemonTimeout * 1000);
    inactivityTimer.watchObject(service.data());
    QObject::connect(&inactivityTimer, SIGNAL(timeout()), &app, SLOT(quit()));

    QDBusConnection bus = QDBusConnection::sessionBus();
    serviceAdaptor->registerAt(bus, FM_RADIO_SERVICE_OBJECT_PATH);
    bool ok = bus.registerService(FM_RADIO_SERVICE_NAME);
    if (Q_UNLIKELY(!ok)) {
        qWarning() << "Failed to register D-Bus name";
        return EXIT_FAILURE;
    }
    bus.connect(QString(),
                QStringLiteral("/org/freedesktop/DBus/Local"),
                QStringLiteral("org.freedesktop.DBus.Local"),
                QStringLiteral("Disconnected"),
                &app, SLOT(quit()));

    int ret = app.exec();

    bus.unregisterService(FM_RADIO_SERVICE_NAME);
    bus.unregisterObject(FM_RADIO_SERVICE_OBJECT_PATH);

    qDebug() << "quitting...";
    return ret;
}
