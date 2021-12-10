/*
 * Copyright (C) 2013-2015 Canonical Ltd.
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

#ifndef FM_RADIO_SERVICE_INACTIVITY_TIMER_H
#define FM_RADIO_SERVICE_INACTIVITY_TIMER_H

#include <QList>
#include <QObject>
#include <QTimer>

namespace FmRadioService {

class InactivityTimer: public QObject
{
    Q_OBJECT

public:
    InactivityTimer(int interval, QObject *parent = 0);
    ~InactivityTimer() {}

    void watchObject(QObject *object);

Q_SIGNALS:
    void timeout();

private Q_SLOTS:
    void onIdleChanged();
    void onTimeout();

private:
    bool allObjectsAreIdle() const;

private:
    QList<QObject*> m_watchedObjects;
    QTimer m_timer;
    int m_interval;
};

} // namespace

#endif // FM_RADIO_SERVICE_INACTIVITY_TIMER_H
