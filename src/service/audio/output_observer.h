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

#ifndef FM_RADIO_SERVICE_AUDIO_OUTPUT_OBSERVER_H
#define FM_RADIO_SERVICE_AUDIO_OUTPUT_OBSERVER_H

#include <QObject>

namespace FmRadioService {
namespace audio {

// All known states of an audio output.
enum class OutputState
{
    // The output is via a private earpiece (i.e. headphones, headset)
    Earpiece,
    // The output is via the internal speaker.
    Speaker,
    // The output is via an external device (a2dp, etc)
    External,
};

class OutputObserverPrivate;
class OutputObserver: public QObject
{
    Q_OBJECT

public:
    OutputObserver(QObject *parent = nullptr);
    virtual ~OutputObserver();

    OutputState outputState() const;
    bool wiredOutputIsAvailable() const;

Q_SIGNALS:
    void outputStateChanged();
    void wiredOutputIsAvailableChanged();

protected:
    OutputObserver(const OutputObserver&) = delete;
    OutputObserver& operator=(const OutputObserver&) = delete;

private:
    Q_DECLARE_PRIVATE(OutputObserver)
    QScopedPointer<OutputObserverPrivate> d_ptr;
};

} // namespace
} // namespace

#endif // FM_RADIO_SERVICE_AUDIO_OUTPUT_OBSERVER_H
