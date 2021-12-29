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

#ifndef FM_RADIO_SERVICE_PLUGIN_RADIO_CONTROL_H
#define FM_RADIO_SERVICE_PLUGIN_RADIO_CONTROL_H

#include <QObject>
#include <QRadioTunerControl>
#include <QScopedPointer>

namespace FmRadioServicePlugin {

class RadioControlPrivate;
class RadioControl: public QRadioTunerControl
{
    Q_OBJECT

public:
    RadioControl(QObject *parent = 0);
    ~RadioControl();

    QRadioTuner::State state() const override;

    void setBand(QRadioTuner::Band b) override;
    QRadioTuner::Band band() const override;
    bool isBandSupported(QRadioTuner::Band b) const override;

    void setFrequency(int frequency) override;
    int frequency() const override;
    int frequencyStep(QRadioTuner::Band b) const override;
    QPair<int,int> frequencyRange(QRadioTuner::Band b) const override;

    void setStereoMode(QRadioTuner::StereoMode mode) override;
    QRadioTuner::StereoMode stereoMode() const override;
    bool isStereo() const override;

    int signalStrength() const override;

    void setVolume(int volume) override;
    int volume() const override;

    void setMuted(bool muted) override;
    bool isMuted() const override;

    bool isAntennaConnected() const override;

    bool isSearching() const override;
    void searchForward() override;
    void searchBackward() override;
    void searchAllStations(QRadioTuner::SearchMode searchMode) override;
    void cancelSearch() override;

    void start() override;
    void stop() override;

    QRadioTuner::Error error() const override;
    QString errorString() const override;

    void emitError(QRadioTuner::Error error);

private:
    Q_DECLARE_PRIVATE(RadioControl)
    QScopedPointer<RadioControlPrivate> d_ptr;
};

} // namespace

#endif // FM_RADIO_SERVICE_PLUGIN_RADIO_CONTROL_H
