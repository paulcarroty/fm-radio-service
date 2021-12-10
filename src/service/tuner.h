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

#ifndef FM_RADIO_SERVICE_TUNER_H
#define FM_RADIO_SERVICE_TUNER_H

#include "error.h"
#include "types.h"

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QVariantMap>

namespace FmRadioService {

class TunerPrivate;
class Tuner: public QObject
{
    Q_OBJECT

public:
    Tuner(QObject *parent = nullptr);
    ~Tuner();

    bool hasValidBackend() const;
    Error lastError() const;

    bool isAntennaConnected() const;
    Band band() const;
    int frequency() const;
    int signalStrength() const;
    int volume() const;
    bool isMuted() const;
    bool isSearching() const;
    bool isStereo() const;
    TunerState state() const;
    QVariantMap metadata() const;

    void setBand(Band band);
    void setFrequency(int frequency);
    void setVolume(int volume);
    void setMuted(bool muted);

    void searchForward();
    void searchBackward();
    void start();
    void stop();
    void close();

Q_SIGNALS:
    void stationFound(int frequency, const QString &stationId);
    void errorOccurred(const Error &error);
    void closed();

    void antennaConnectedChanged();
    void bandChanged();
    void frequencyChanged();
    void mutedChanged();
    void searchingChanged();
    void signalStrengthChanged();
    void stateChanged();
    void stereoChanged();
    void volumeChanged();

private:
    Q_DECLARE_PRIVATE(Tuner)
    QScopedPointer<TunerPrivate> d_ptr;
};

} // namespace

#endif // FM_RADIO_SERVICE_TUNER_H
