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

#include "playback.h"

#include "context.h"
#include "error.h"
#include "operation.h"

#include <QDebug>
#include <QRegularExpression>
#include <pulse/pulseaudio.h>

using namespace FmRadioService::audio;

namespace FmRadioService {
namespace audio {

class PlaybackPrivate
{
    Q_DECLARE_PUBLIC(Playback)

public:
    PlaybackPrivate(Playback *q);
    ~PlaybackPrivate();

    void onContextReady();

    void subscriptionCb(pa_subscription_event_type_t t, uint32_t idx);

    void getSourceInfoList();
    void getSourceInfoListCb(const pa_source_info *i, bool isLast);
    static void get_source_info_list_cb(pa_context *, const pa_source_info *i,
                                      int eol, void *userdata) {
        if (auto thiz = static_cast<PlaybackPrivate*>(userdata)) {
            thiz->getSourceInfoListCb(i, eol != 0);
        }
    }

    void loadModuleCb(uint32_t index);
    static void load_module_cb(pa_context *, uint32_t idx, void *userdata) {
        if (auto thiz = static_cast<PlaybackPrivate*>(userdata)) {
            thiz->loadModuleCb(idx);
        }
    }

    void setVolume(int volume);
    void start();
    void stop();

private:
    QSharedPointer<Context> m_contextHolder;
    pa_context *m_context;
    Subscription m_subscription;
    QRegularExpression m_radioSourcePortRegexp;
    QByteArray m_radioSourceName;
    QByteArray m_radioSourcePortName;
    QByteArray m_oldActivePort;
    Operation m_sourceListOp;
    Operation m_loadModuleOp;
    QByteArray m_defaultSinkName;
    QByteArray m_defaultSourceName;
    uint32_t m_moduleIndex = PA_INVALID_INDEX;
    bool m_stopRequested = false;
    int m_queuedVolume = -1;
    Playback *q_ptr;
};

}} // namespace

PlaybackPrivate::PlaybackPrivate(Playback *q):
    m_contextHolder(Context::instance()),
    m_context(m_contextHolder->get()),
    q_ptr(q)
{
    m_contextHolder->onReady([this]() { onContextReady(); });
}

PlaybackPrivate::~PlaybackPrivate()
{
}

void PlaybackPrivate::onContextReady()
{
    m_subscription =
        m_contextHolder->subscribe(PA_SUBSCRIPTION_MASK_SOURCE,
                                   [this](pa_subscription_event_type_t t,
                                          uint32_t idx) {
        subscriptionCb(t, idx);
    });

    Operation op =
        pa_context_subscribe(m_context, PA_SUBSCRIPTION_MASK_ALL,
                             nullptr, nullptr);

    getSourceInfoList();
}

void PlaybackPrivate::subscriptionCb(pa_subscription_event_type_t t,
                                           uint32_t idx)
{
    qDebug() << Q_FUNC_INFO << t << idx;
    if (t & PA_SUBSCRIPTION_EVENT_SOURCE) {
        getSourceInfoList();
    }
}

void PlaybackPrivate::getSourceInfoList()
{
    m_sourceListOp =
        pa_context_get_source_info_list(m_context,
                                        get_source_info_list_cb, this);
}

void PlaybackPrivate::getSourceInfoListCb(const pa_source_info *si,
                                          bool isLast)
{
    if (isLast) {
        return;
    }
    bool isRadioSource = false;
    for (uint32_t i = 0; i < si->n_ports; i++) {
        const pa_source_port_info *pi = si->ports[i];
        auto match = m_radioSourcePortRegexp.match(pi->name);
        if (match.hasMatch()) {
            qDebug() << "Found port" << pi->name << "on source" << si->name;
            isRadioSource = true;
            m_radioSourceName = si->name;
            m_radioSourcePortName = pi->name;
            break;
        }
    }

    if (isRadioSource) {
        QByteArray activePort;
        if (si->active_port) {
            activePort = si->active_port->name;
        }

        if (!activePort.isEmpty() &&
            activePort != m_radioSourcePortName) {
            m_oldActivePort = activePort;
        }
    }
}

void PlaybackPrivate::loadModuleCb(uint32_t index)
{
    Q_Q(Playback);
    qDebug() << "module loaded with index" << index;
    m_moduleIndex = index;
    if (index != PA_INVALID_INDEX) {
        Q_EMIT q->started();
    } else {
        Error error(Error::TunerError,
                    "Cannot load pulseaudio loopback module");
        Q_EMIT q->errorOccurred(error);
        // continue in case we need to execute a stop() call
    }

    if (m_stopRequested) {
        stop();
    }
}

void PlaybackPrivate::setVolume(int volume)
{
    if (m_radioSourceName.isEmpty()) {
        m_queuedVolume = volume;
        return;
    }

    pa_cvolume vol;
    pa_cvolume_init(&vol);
    pa_cvolume_set(&vol, 1, PA_VOLUME_NORM * volume / 100);
    Operation op =
        pa_context_set_source_volume_by_name(m_context,
                                             m_radioSourceName,
                                             &vol,
                                             nullptr, nullptr);
}

void PlaybackPrivate::start()
{
    Q_Q(Playback);

    qDebug() << Q_FUNC_INFO;
    if (m_stopRequested) {
        Error error(Error::ResourceBusy,
                    "Still waiting for a previous operation");
        Q_EMIT q->errorOccurred(error);
        return;
    }
    qDebug() << "source:" << m_radioSourceName;
    qDebug() << "port:" << m_radioSourcePortName;
    qDebug() << "old active port:" << m_oldActivePort;
    if (m_radioSourcePortName.isEmpty()) {
        Error error(Error::AudioError, "FM source port not found");
        Q_EMIT q->errorOccurred(error);
        return;
    }
    Operation op =
        pa_context_set_source_port_by_name(m_context,
                                           m_radioSourceName,
                                           m_radioSourcePortName,
                                           nullptr, nullptr);
    QByteArray options =
        "sink_input_properties=media.role=multimedia";
    m_loadModuleOp =
        pa_context_load_module(m_context,
                               "module-loopback",
                               options.constData(),
                               load_module_cb, this);
}

void PlaybackPrivate::stop()
{
    Q_Q(Playback);

    qDebug() << Q_FUNC_INFO;
    if (m_loadModuleOp.isRunning()) {
        /*We must first wait for it to complete */
        m_stopRequested = true;
        return;
    }

    if (m_moduleIndex != PA_INVALID_INDEX) {
        Operation op =
            pa_context_unload_module(m_context,
                                     m_moduleIndex,
                                     nullptr, nullptr);
        m_moduleIndex = PA_INVALID_INDEX;
    }

    if (!m_oldActivePort.isEmpty()) {
        Operation op =
            pa_context_set_source_port_by_name(m_context,
                                               m_radioSourceName,
                                               m_oldActivePort,
                                               nullptr, nullptr);
    }
    m_stopRequested = false;
    Q_EMIT q->stopped();
}

Playback::Playback(QObject *parent):
    QObject(parent),
    d_ptr(new PlaybackPrivate(this))
{
}

Playback::~Playback() = default;

void Playback::setRadioPortPattern(const QString &pattern)
{
    Q_D(Playback);
    d->m_radioSourcePortRegexp.setPattern(pattern);
}

void Playback::setVolume(int volume)
{
    Q_D(Playback);
    d->setVolume(volume);
}

void Playback::start()
{
    Q_D(Playback);
    d->start();
}

void Playback::stop()
{
    Q_D(Playback);
    d->stop();
}
