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

#include "tuner_adaptor.h"

#include "dbus_property_notifier.h"
#include "error.h"
#include "tuner.h"

#include <QCoreApplication>
#include <QDBusMessage>

using namespace FmRadioService;

namespace FmRadioService {

class TunerAdaptorPrivate
{
    Q_DECLARE_PUBLIC(TunerAdaptor)

public:
    TunerAdaptorPrivate(Tuner *tuner, const QString &clientId,
                        TunerAdaptor *q);

    bool isSameClient() const;

    static QString errorCodeString(const Error &error);

    void sendErrorReply(const Error &error) const;
    void sendNoBackendError() const;

private:
    Tuner *m_tuner;
    QScopedPointer<DBusPropertyNotifier> m_propertyNotifier;
    QString m_clientId;
    TunerAdaptor *q_ptr;
};

} // namespace

static DBusConstants::Band bandToDBus(Band band)
{
    return static_cast<DBusConstants::Band>(band);
}

static Band bandFromDBus(quint32 band)
{
    return static_cast<Band>(band);
}

static DBusConstants::TunerState stateToDBus(TunerState state)
{
    return static_cast<DBusConstants::TunerState>(state);
}

TunerAdaptorPrivate::TunerAdaptorPrivate(Tuner *tuner,
                                         const QString &clientId,
                                         TunerAdaptor *q):
    m_tuner(tuner),
    m_clientId(clientId),
    q_ptr(q)
{
    QObject::connect(tuner, &Tuner::stationFound,
                     q, &TunerAdaptor::StationFound);
    QObject::connect(tuner, &Tuner::errorOccurred,
                     q, [this, q](const Error &error) {
        if (q->calledFromDBus()) {
            sendErrorReply(error);
        } else {
            Q_EMIT q->Error(errorCodeString(error), error.message());
        }
    });

    QObject::connect(tuner, &Tuner::antennaConnectedChanged,
                     q, &TunerAdaptor::antennaConnectedChanged);
    QObject::connect(tuner, &Tuner::bandChanged,
                     q, &TunerAdaptor::bandChanged);
    QObject::connect(tuner, &Tuner::frequencyChanged,
                     q, &TunerAdaptor::frequencyChanged);
    QObject::connect(tuner, &Tuner::mutedChanged,
                     q, &TunerAdaptor::mutedChanged);
    QObject::connect(tuner, &Tuner::searchingChanged,
                     q, &TunerAdaptor::searchingChanged);
    QObject::connect(tuner, &Tuner::signalStrengthChanged,
                     q, &TunerAdaptor::signalStrengthChanged);
    QObject::connect(tuner, &Tuner::stateChanged,
                     q, &TunerAdaptor::stateChanged);
    QObject::connect(tuner, &Tuner::stereoChanged,
                     q, &TunerAdaptor::stereoChanged);
    QObject::connect(tuner, &Tuner::volumeChanged,
                     q, &TunerAdaptor::volumeChanged);
}

bool TunerAdaptorPrivate::isSameClient() const
{
    Q_Q(const TunerAdaptor);
    return !q->calledFromDBus() || q->message().service() == m_clientId;
}

QString TunerAdaptorPrivate::errorCodeString(const Error &error)
{
    QString code;
    switch (error.code()) {
    case Error::ResourceError:
        code = FM_RADIO_SERVICE_ERROR_PREFIX FM_RADIO_SERVICE_ERROR_RESOURCE;
        break;
    default:
        qWarning() << "Unknown error code" << error.code();
        code = FDO_ERROR_PREFIX FDO_ERROR_FAILED;
    }
    return code;
}

void TunerAdaptorPrivate::sendErrorReply(const Error &error) const
{
    Q_Q(const TunerAdaptor);

    QString code = errorCodeString(error);
    q->sendErrorReply(code, error.message());
}

static void sendInvalidClientError(const QDBusContext *c)
{
    c->sendErrorReply(FDO_ERROR_PREFIX FDO_ERROR_ACCESS_DENIED,
                      QStringLiteral("Tuner belongs to another client"));
}

#define RETURN_IF_INVALID_CLIENT(value) \
    { \
        Q_D(const TunerAdaptor); \
        if (!d->isSameClient()) { \
            sendInvalidClientError(this); \
            return value; \
        } \
    }

#define FAIL_IF_INVALID_CLIENT() \
    { \
        Q_D(const TunerAdaptor); \
        if (!d->isSameClient()) { \
            sendInvalidClientError(this); \
            return; \
        } \
    }

TunerAdaptor::TunerAdaptor(Tuner *tuner, const QString &clientId,
                           QObject *parent):
    QObject(parent),
    d_ptr(new TunerAdaptorPrivate(tuner, clientId, this))
{
}

TunerAdaptor::~TunerAdaptor() = default;

bool TunerAdaptor::registerAt(QDBusConnection &connection,
                              const QString &objectPath)
{
    Q_D(TunerAdaptor);
    d->m_propertyNotifier.reset(new DBusPropertyNotifier(connection,
                                                         objectPath, this));
    return connection.registerObject(
            objectPath,
            this,
            QDBusConnection::ExportAllSlots |
            QDBusConnection::ExportScriptableSignals |
            QDBusConnection::ExportAllProperties);
}

QString TunerAdaptor::clientId() const
{
    Q_D(const TunerAdaptor);
    return d->m_clientId;
}

Tuner *TunerAdaptor::tuner()
{
    Q_D(TunerAdaptor);
    return d->m_tuner;
}

const Tuner *TunerAdaptor::tuner() const
{
    Q_D(const TunerAdaptor);
    return d->m_tuner;
}

bool TunerAdaptor::isAntennaConnected() const
{
    RETURN_IF_INVALID_CLIENT(false);
    return tuner()->isAntennaConnected();
}

quint32 TunerAdaptor::band() const
{
    RETURN_IF_INVALID_CLIENT(DBusConstants::Band::Unset);
    return bandToDBus(tuner()->band());
}

quint32 TunerAdaptor::frequency() const
{
    RETURN_IF_INVALID_CLIENT(0);
    return tuner()->frequency();
}

quint32 TunerAdaptor::signalStrength() const
{
    RETURN_IF_INVALID_CLIENT(0);
    return tuner()->signalStrength();
}

quint32 TunerAdaptor::volume() const
{
    RETURN_IF_INVALID_CLIENT(0);
    return tuner()->volume();
}

bool TunerAdaptor::isMuted() const
{
    RETURN_IF_INVALID_CLIENT(false);
    return tuner()->isMuted();
}

bool TunerAdaptor::isSearching() const
{
    RETURN_IF_INVALID_CLIENT(false);
    return tuner()->isSearching();
}

bool TunerAdaptor::isStereo() const
{
    RETURN_IF_INVALID_CLIENT(false);
    return tuner()->isStereo();
}

quint32 TunerAdaptor::state() const
{
    RETURN_IF_INVALID_CLIENT(0);
    return stateToDBus(tuner()->state());
}

QVariantMap TunerAdaptor::metadata() const
{
    RETURN_IF_INVALID_CLIENT(QVariantMap());
    return tuner()->metadata();
}

void TunerAdaptor::setBand(quint32 band)
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->setBand(bandFromDBus(band));
}

void TunerAdaptor::setFrequency(quint32 frequency)
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->setFrequency(frequency);
}

void TunerAdaptor::setVolume(quint32 volume)
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->setVolume(volume);
}

void TunerAdaptor::setMuted(bool muted)
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->setMuted(muted);
}

void TunerAdaptor::SearchBackward()
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->searchBackward();
}

void TunerAdaptor::SearchForward()
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->searchForward();
}

void TunerAdaptor::Start()
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->start();
}

void TunerAdaptor::Stop()
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->stop();
}

void TunerAdaptor::Close()
{
    FAIL_IF_INVALID_CLIENT();
    tuner()->close();
    deleteLater();
}
