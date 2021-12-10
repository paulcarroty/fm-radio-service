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

#ifndef FM_RADIO_SERVICE_SERVICE_H
#define FM_RADIO_SERVICE_SERVICE_H

#include "error.h"
#include "types.h"

#include <QObject>
#include <QScopedPointer>
#include <QString>

namespace FmRadioService {

class Tuner;

class ServicePrivate;
class Service: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isIdle READ isIdle NOTIFY isIdleChanged)

public:
    Service(QObject *parent = nullptr);
    ~Service();

    Error lastError() const;

    bool isIdle() const;

    Tuner *openTuner();

Q_SIGNALS:
    void isIdleChanged();

private:
    Q_DECLARE_PRIVATE(Service)
    QScopedPointer<ServicePrivate> d_ptr;
};

} // namespace

#endif // FM_RADIO_SERVICE_SERVICE_H
