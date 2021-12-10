/*
 * Copyright © 2021 Canonical Ltd.
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

#ifndef FM_RADIO_SERVICE_AUDIO_PLAYBACK_H
#define FM_RADIO_SERVICE_AUDIO_PLAYBACK_H

#include <QByteArray>
#include <QObject>
#include <QString>

namespace FmRadioService {

class Error;

namespace audio {

class PlaybackPrivate;
class Playback: public QObject
{
    Q_OBJECT

public:
    Playback(QObject *parent = nullptr);
    virtual ~Playback();

    void setRadioPortPattern(const QString &pattern);

    void setVolume(int volume);
    int volume() const;

    void start();
    void stop();

Q_SIGNALS:
    void started();
    void stopped();
    void errorOccurred(const Error &error);

protected:
    Playback(const Playback&) = delete;
    Playback& operator=(const Playback&) = delete;

private:
    Q_DECLARE_PRIVATE(Playback)
    QScopedPointer<PlaybackPrivate> d_ptr;
};

} // namespace
} // namespace

#endif // FM_RADIO_SERVICE_AUDIO_OUTPUT_OBSERVER_H
