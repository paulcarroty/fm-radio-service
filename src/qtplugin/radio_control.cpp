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

#include "radio_control.h"

#include "dbus_constants.h"
#include "error.h"
#include "types.h"

#include <QDBusAbstractInterface>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>

using namespace FmRadioServicePlugin;

typedef std::function<void(const QDBusMessage &)> MethodCb;
typedef std::function<void()> VoidMethodCb;

class DBusService: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    DBusService():
        QDBusAbstractInterface(QStringLiteral(FM_RADIO_SERVICE_NAME),
                               QStringLiteral(FM_RADIO_SERVICE_OBJECT_PATH),
                               FM_RADIO_SERVICE_INTERFACE,
                               QDBusConnection::sessionBus(),
                               nullptr) {
    }

    QDBusMessage openTuner() {
        return call(QStringLiteral("OpenTuner"));
    }
};

class DBusTuner: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    DBusTuner(const QDBusConnection &conn, const QString &path):
        QDBusAbstractInterface(QStringLiteral(FM_RADIO_SERVICE_NAME),
                               path,
                               FM_RADIO_SERVICE_TUNER_INTERFACE,
                               conn,
                               nullptr) {
    }

};

namespace FmRadioServicePlugin {

class RadioControlPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(RadioControl)

public:
    RadioControlPrivate(RadioControl *q);

    void updateProperties(const QVariantMap &properties);
    void watchErrors(const QDBusPendingCall &call);
    void onSuccessfulCompletion(const QDBusPendingCall &call,
                                MethodCb callback);
    void call(const QString &method,
              const QVariant &arg1 = QVariant(),
              const QVariant &arg2 = QVariant());
    void setProperty(const QString &name, const QVariant &value,
                     VoidMethodCb callback = [](){});

private Q_SLOTS:
    void onPropertiesChanged(const QString &interface,
                             const QVariantMap &changed,
                             const QStringList &invalidated);

private:
    DBusService m_service;
    QScopedPointer<DBusTuner> m_tuner;
    QRadioTuner::State m_state = QRadioTuner::StoppedState;
    QRadioTuner::Band m_band = QRadioTuner::FM;
    Error m_lastError;
    int m_freqMin = 0;
    int m_freqMax = 0;
    int m_frequency = 0;
    int m_volume = 0;  // from 0 to 100
    int m_signalStrength = 0;
    bool m_isStereo = false;
    bool m_isMuted = false;
    bool m_isSearching = false;
    bool m_isAntennaConnected = false;
    QVariantMap m_metaData;
    RadioControl *q_ptr;
};

} // namespace

RadioControlPrivate::RadioControlPrivate(RadioControl *q):
    q_ptr(q)
{
    QDBusMessage reply = m_service.openTuner();
    if (Q_UNLIKELY(reply.type() == QDBusMessage::ErrorMessage)) {
        qWarning() << "Failed to create session:" << reply.errorMessage();
        m_lastError = Error::fromDBus(reply);
        return;
    }
    const QString path = reply.arguments()[0].value<QDBusObjectPath>().path();
    qDebug() << "Got object path" << path;
    m_tuner.reset(new DBusTuner(m_service.connection(), path));

    QDBusConnection c(m_service.connection());

    const QString service = m_tuner->service();
    const QString interface = m_tuner->interface();

    c.connect(service, path, QStringLiteral(FDO_PROPERTIES_INTERFACE),
              QStringLiteral("PropertiesChanged"),
              this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));

    // Blocking call to get the initial properties
    QDBusMessage msg = QDBusMessage::createMethodCall(
        service, path,
        QStringLiteral(FDO_PROPERTIES_INTERFACE),
        QStringLiteral("GetAll"));
    msg.setArguments({ interface });
    reply = c.call(msg);
    if (Q_UNLIKELY(reply.type()) == QDBusMessage::ErrorMessage) {
        qWarning() << "Cannot get tuner properties:" << reply.errorMessage();
    } else {
        QDBusArgument arg = reply.arguments().first().value<QDBusArgument>();
        updateProperties(qdbus_cast<QVariantMap>(arg));
    }
}

void RadioControlPrivate::updateProperties(const QVariantMap &properties)
{
    Q_Q(RadioControl);

    for (auto i = properties.begin(); i != properties.end(); i++) {
        const QString &name = i.key();
        if (name == "AntennaConnected") {
            m_isAntennaConnected = i.value().toBool();
            Q_EMIT q->antennaConnectedChanged(m_isAntennaConnected);
        } else if (name == "Band") {
            m_band = Types::bandFromDBus(i.value().toUInt());
            Q_EMIT q->bandChanged(m_band);
        } else if (name == "Frequency") {
            m_frequency = Types::frequencyFromDBus(i.value().toUInt());
            Q_EMIT q->frequencyChanged(m_frequency);
        } else if (name == "SignalStrength") {
            m_signalStrength = i.value().toUInt();
            Q_EMIT q->signalStrengthChanged(m_signalStrength);
        } else if (name == "Volume") {
            m_volume = i.value().toUInt();
            Q_EMIT q->volumeChanged(m_volume);
        } else if (name == "Muted") {
            m_isMuted = i.value().toBool();
            Q_EMIT q->mutedChanged(m_isMuted);
        } else if (name == "Searching") {
            m_isSearching = i.value().toBool();
            Q_EMIT q->searchingChanged(m_isSearching);
        } else if (name == "Stereo") {
            m_isStereo = i.value().toBool();
            Q_EMIT q->stereoStatusChanged(m_isStereo);
        } else if (name == "State") {
            m_state = Types::stateFromDBus(i.value().toUInt());
            Q_EMIT q->stateChanged(m_state);
        } else if (name == "Metadata") {
            QDBusArgument arg = i.value().value<QDBusArgument>();
            m_metaData = qdbus_cast<QVariantMap>(arg);
            // TODO deliver this via QRadioDataControl
        }
    }
}

void RadioControlPrivate::watchErrors(const QDBusPendingCall &call)
{
    auto watcher = new QDBusPendingCallWatcher(call);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     this, [this](QDBusPendingCallWatcher *call) {
        Q_Q(RadioControl);
        call->deleteLater();
        QDBusPendingReply<void> reply(*call);
        if (reply.isError()) {
            m_lastError = Error::fromDBus(reply.reply());
            q->emitError(m_lastError.code());
        }
    });
}

void RadioControlPrivate::onSuccessfulCompletion(const QDBusPendingCall &call,
                                                 MethodCb callback)
{
    auto watcher = new QDBusPendingCallWatcher(call);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     this, [this, callback](QDBusPendingCallWatcher *call) {
        Q_Q(RadioControl);
        call->deleteLater();
        const QDBusMessage reply = QDBusPendingReply<void>(*call).reply();
        if (Q_UNLIKELY(reply.type()) == QDBusMessage::ErrorMessage) {
            m_lastError = Error::fromDBus(reply);
            q->emitError(m_lastError.code());
        } else if (callback) {
            callback(reply);
        }
    });
}

void RadioControlPrivate::call(const QString &method,
                               const QVariant &arg1, const QVariant &arg2)
{
    QDBusPendingCall call = m_tuner->asyncCall(method, arg1, arg2);
    watchErrors(call);
}

void RadioControlPrivate::setProperty(const QString &name,
                                      const QVariant &value,
                                      VoidMethodCb callback)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_tuner->service(),
        m_tuner->path(),
        QStringLiteral(FDO_PROPERTIES_INTERFACE),
        QStringLiteral("Set"));
    msg.setArguments({
        QStringLiteral(FM_RADIO_SERVICE_TUNER_INTERFACE),
        name,
        QVariant::fromValue(QDBusVariant(value)),
    });
    QDBusPendingCall call = m_tuner->connection().asyncCall(msg);
    onSuccessfulCompletion(call, [callback](const QDBusMessage &) {
        callback();
    });
}

void RadioControlPrivate::onPropertiesChanged(const QString &interface,
                                              const QVariantMap &changed,
                                              const QStringList &invalidated)
{
    if (interface == QStringLiteral(FM_RADIO_SERVICE_TUNER_INTERFACE)) {
        updateProperties(changed);
        // we know that our service will never invalidate properties
        Q_UNUSED(invalidated);
    }
}

RadioControl::RadioControl(QObject *parent):
    QRadioTunerControl(parent),
    d_ptr(new RadioControlPrivate(this))
{
}

RadioControl::~RadioControl() = default;

QRadioTuner::State RadioControl::state() const
{
    Q_D(const RadioControl);
    return d->m_state;
}

void RadioControl::setBand(QRadioTuner::Band band)
{
    Q_D(RadioControl);

    if (Q_UNLIKELY(!isBandSupported(band))) {
        qWarning() << "Unsupported band" << band;
        return;  // Should we emit an error?
    }

    if (band != d->m_band) {
        d->setProperty(QStringLiteral("Band"), Types::bandToDBus(band));
    }
}

QRadioTuner::Band RadioControl::band() const
{
    Q_D(const RadioControl);
    return d->m_band;
}

bool RadioControl::isBandSupported(QRadioTuner::Band band) const
{
    Q_D(const RadioControl);

    int freqMin = d->m_freqMin;
    int freqMax = d->m_freqMax;
    switch (band) {
    case QRadioTuner::FM:
        return freqMin <= 87500000 && freqMax >= 108000000;
    case QRadioTuner::LW:
        return freqMin <= 148500 && freqMax >= 283500;
    case QRadioTuner::AM:
        return freqMin <= 520000 && freqMax >= 1610000;
    case QRadioTuner::SW:
        return freqMin <= 1711000 && freqMax >= 30000000;
    default:
        return false;
    }
}

void RadioControl::setFrequency(int frequency)
{
    Q_D(RadioControl);
    if (frequency == d->m_frequency) return;
    d->setProperty(QStringLiteral("Frequency"),
                   Types::frequencyToDBus(frequency));
}

int RadioControl::frequency() const
{
    Q_D(const RadioControl);
    return d->m_frequency;
}

int RadioControl::frequencyStep(QRadioTuner::Band band) const
{
    switch (band) {
    case QRadioTuner::FM:
        return 100000;
    case QRadioTuner::LW:
        return 1000;
    case QRadioTuner::AM:
        return 1000;
    case QRadioTuner::SW:
        return 500;
    default:
        return 0;
    }
}

QPair<int,int> RadioControl::frequencyRange(QRadioTuner::Band band) const
{
    switch (band) {
    case QRadioTuner::FM:
        return { 87500000, 108000000 };
    case QRadioTuner::LW:
        return { 148500, 283500 };
    case QRadioTuner::AM:
        return { 520000, 1710000 };
    case QRadioTuner::SW:
        return { 1711111, 30000000 };
    default:
        return { 0, 0 };
    }
}

void RadioControl::setStereoMode(QRadioTuner::StereoMode mode)
{
    Q_UNUSED(mode);
    // TODO
    qWarning() << Q_FUNC_INFO << "not implemented";
}

QRadioTuner::StereoMode RadioControl::stereoMode() const
{
    return QRadioTuner::Auto;
}

bool RadioControl::isStereo() const
{
    Q_D(const RadioControl);
    return d->m_isStereo;
}

int RadioControl::signalStrength() const
{
    Q_D(const RadioControl);
    return d->m_signalStrength;
}

void RadioControl::setVolume(int volume)
{
    Q_D(RadioControl);
    if (volume == d->m_volume) return;
    d->setProperty(QStringLiteral("Volume"), quint32(volume));
}

int RadioControl::volume() const
{
    Q_D(const RadioControl);
    return d->m_volume;
}

void RadioControl::setMuted(bool muted)
{
    Q_D(RadioControl);
    if (muted == d->m_isMuted) return;
    d->setProperty(QStringLiteral("Muted"), muted);
}

bool RadioControl::isMuted() const
{
    Q_D(const RadioControl);
    return d->m_isMuted;
}

bool RadioControl::isAntennaConnected() const
{
    Q_D(const RadioControl);
    return d->m_isAntennaConnected;
}

bool RadioControl::isSearching() const
{
    Q_D(const RadioControl);
    return d->m_isSearching;
}

void RadioControl::searchForward()
{
    Q_D(RadioControl);
    if (d->m_isSearching) {
        cancelSearch();
    }
    d->call(QStringLiteral("SearchForward"));
}

void RadioControl::searchBackward()
{
    Q_D(RadioControl);
    if (d->m_isSearching) {
        cancelSearch();
    }
    d->call(QStringLiteral("SearchBackward"));
}

void RadioControl::searchAllStations(QRadioTuner::SearchMode searchMode)
{
    Q_UNUSED(searchMode);
    // TODO
    qWarning() << Q_FUNC_INFO << "not implemented";
}

void RadioControl::cancelSearch()
{
    // TODO
    qWarning() << Q_FUNC_INFO << "not implemented";
}

void RadioControl::start()
{
    Q_D(RadioControl);
    if (d->m_state == QRadioTuner::ActiveState) {
        qWarning() << "Already in active state";
        return;
    }
    d->call(QStringLiteral("Start"));
}

void RadioControl::stop()
{
    Q_D(RadioControl);
    if (d->m_state == QRadioTuner::StoppedState) {
        qWarning() << "Already in stopped state";
        return;
    }
    d->call(QStringLiteral("Stop"));
}

QRadioTuner::Error RadioControl::error() const
{
    Q_D(const RadioControl);
    return d->m_lastError.code();
}

QString RadioControl::errorString() const
{
    Q_D(const RadioControl);
    return d->m_lastError.message();
}

void RadioControl::emitError(QRadioTuner::Error err)
{
    // Hack, because emitting errors directly from the private class does not
    // work
    Q_EMIT QRadioTunerControl::error(err);
}

#include "radio_control.moc"
