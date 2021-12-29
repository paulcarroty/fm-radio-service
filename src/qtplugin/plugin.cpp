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

#include "plugin.h"

#include "radio_service.h"

#include <QDebug>

QMediaService* Plugin::create(QString const &key)
{
    qDebug() << Q_FUNC_INFO << "fmradio" << key;
    if (key == QStringLiteral("org.qt-project.qt.radio")) {
        return new FmRadioServicePlugin::RadioService();
    }
    return 0;
}

void Plugin::release(QMediaService *service)
{
    delete service;
}

QList<QByteArray> Plugin::devices(const QByteArray &service) const
{
    qDebug() << Q_FUNC_INFO << "fmradio" << service;
    return QList<QByteArray>();
}

QString Plugin::deviceDescription(const QByteArray &service,
                                  const QByteArray &device)
{
    qDebug() << Q_FUNC_INFO << "fmradio" << service << device;
    return QString();
}
