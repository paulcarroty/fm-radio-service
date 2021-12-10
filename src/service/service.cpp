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

#include "service.h"

#include "tuner.h"

#include <QDebug>

using namespace FmRadioService;

namespace FmRadioService {

class ServicePrivate
{
    Q_DECLARE_PUBLIC(Service)

public:
    ServicePrivate(Service *q);

    Tuner *openTuner();

    void increaseUsageCount();
    void decreaseUsageCount();

    void clearError() { m_lastError = Error(); }

private:
    QScopedPointer<Tuner> m_tuner;
    Error m_lastError;
    int m_usageCount = 0;
    Service *q_ptr;
};

} // namespace

ServicePrivate::ServicePrivate(Service *q):
    m_tuner(new Tuner),
    q_ptr(q)
{
    QObject::connect(m_tuner.data(), &Tuner::closed,
                     q, [this]() {
        decreaseUsageCount();
    });
}

Tuner *ServicePrivate::openTuner()
{
    clearError();
    if (!m_tuner->hasValidBackend()) {
        m_lastError = Error(Error::ResourceError,
                            "Radio backend not available");
        return nullptr;
    }
    if (m_usageCount > 0) {
        m_lastError = Error(Error::ResourceBusy,
                            "The tuner is being used by another application");
        return nullptr;
    }
    increaseUsageCount();
    return m_tuner.data();
}

void ServicePrivate::increaseUsageCount()
{
    m_usageCount++;
    if (m_usageCount == 1) {
        Q_Q(Service);
        Q_EMIT q->isIdleChanged();
    }
}

void ServicePrivate::decreaseUsageCount()
{
    if (Q_UNLIKELY(m_usageCount <= 0)) {
        qWarning() << "Impossible situation: usage count going below 0!";
        return;
    }
    m_usageCount--;
    if (m_usageCount == 0) {
        Q_Q(Service);
        Q_EMIT q->isIdleChanged();
    }
}

Service::Service(QObject *parent):
    QObject(parent),
    d_ptr(new ServicePrivate(this))
{
}

Service::~Service() = default;

Error Service::lastError() const
{
    Q_D(const Service);
    return d->m_lastError;
}

bool Service::isIdle() const
{
    Q_D(const Service);
    return d->m_usageCount == 0;
}

Tuner *Service::openTuner()
{
    Q_D(Service);
    return d->openTuner();
}
