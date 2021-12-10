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

#ifndef FM_RADIO_SERVICE_TUNER_ADAPTOR_H
#define FM_RADIO_SERVICE_TUNER_ADAPTOR_H

#include "dbus_constants.h"

#include <QDBusConnection>
#include <QDBusContext>
#include <QString>
#include <QVariantMap>

namespace FmRadioService
{

class Tuner;

class TunerAdaptorPrivate;
class TunerAdaptor: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", FM_RADIO_SERVICE_TUNER_INTERFACE)
    Q_PROPERTY(bool AntennaConnected READ isAntennaConnected
               NOTIFY antennaConnectedChanged)
    // values are defined by DBusConstants::Band
    Q_PROPERTY(quint32 Band READ band WRITE setBand NOTIFY bandChanged)
    Q_PROPERTY(quint32 Frequency READ frequency WRITE setFrequency
               NOTIFY frequencyChanged)
    Q_PROPERTY(quint32 SignalStrength READ signalStrength
               NOTIFY signalStrengthChanged)
    Q_PROPERTY(quint32 Volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool Muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool Searching READ isSearching NOTIFY searchingChanged)
    Q_PROPERTY(bool Stereo READ isStereo NOTIFY stereoChanged)
    Q_PROPERTY(quint32 State READ state NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap Metadata READ metadata NOTIFY metadataChanged)
    // TODO: add a constant property with supported bands and frequencies

public:
    TunerAdaptor(Tuner *tuner, const QString &clientId,
                 QObject *parent = nullptr);
    ~TunerAdaptor();

    bool registerAt(QDBusConnection &connection, const QString &objectPath);
    QString clientId() const;

    Tuner *tuner();
    const Tuner *tuner() const;

    bool isAntennaConnected() const;
    quint32 band() const;
    quint32 frequency() const;
    quint32 signalStrength() const;
    quint32 volume() const;
    bool isMuted() const;
    bool isSearching() const;
    bool isStereo() const;
    quint32 state() const;
    QVariantMap metadata() const;

    void setBand(quint32 band);
    void setFrequency(quint32 frequency);
    void setVolume(quint32 volume);
    void setMuted(bool muted);

public Q_SLOTS:
    void SearchBackward();
    void SearchForward();
    void Start();
    void Stop();
    void Close();

Q_SIGNALS:
    Q_SCRIPTABLE void StationFound(quint32 frequency, const QString &stationId);
    Q_SCRIPTABLE void Error(const QString &code, const QString &message);

    void antennaConnectedChanged();
    void bandChanged();
    void frequencyChanged();
    void signalStrengthChanged();
    void volumeChanged();
    void mutedChanged();
    void searchingChanged();
    void stereoChanged();
    void stateChanged();
    void metadataChanged();

private:
    Q_DECLARE_PRIVATE(TunerAdaptor)
    QScopedPointer<TunerAdaptorPrivate> d_ptr;
};

} // namespace

#endif // FM_RADIO_SERVICE_TUNER_ADAPTOR_H
