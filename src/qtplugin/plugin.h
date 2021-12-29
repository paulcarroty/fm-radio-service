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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QMediaServiceProviderPlugin>

class Plugin: public QMediaServiceProviderPlugin,
              public QMediaServiceSupportedDevicesInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedDevicesInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0"
                      FILE "fmradio.json")

public:
    QMediaService *create(QString const &key) override;
    void release(QMediaService *service) override;

    QList<QByteArray> devices(const QByteArray &service) const override;
    QString deviceDescription(const QByteArray &service,
                              const QByteArray &device) override;
};

#endif // PLUGIN_H
