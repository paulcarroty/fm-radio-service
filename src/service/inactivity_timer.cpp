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

#include "inactivity_timer.h"

#include <QVariant>

using namespace FmRadioService;

InactivityTimer::InactivityTimer(int interval, QObject *parent):
    QObject(parent),
    m_interval(interval)
{
    m_timer.setSingleShot(true);
    QObject::connect(&m_timer, SIGNAL(timeout()),
                     this, SLOT(onTimeout()));
}

void InactivityTimer::watchObject(QObject *object)
{
    connect(object, SIGNAL(isIdleChanged()), SLOT(onIdleChanged()));
    m_watchedObjects.append(object);

    /* Force an initial check */
    onIdleChanged();
}

void InactivityTimer::onIdleChanged()
{
    if (allObjectsAreIdle()) {
        m_timer.start(m_interval);
    } else {
        m_timer.stop();
    }
}

void InactivityTimer::onTimeout()
{
    if (allObjectsAreIdle()) {
        Q_EMIT timeout();
    }
}

bool InactivityTimer::allObjectsAreIdle() const
{
    Q_FOREACH(const QObject *object, m_watchedObjects) {
        if (!object->property("isIdle").toBool()) {
            return false;
        }
    }
    return true;
}
