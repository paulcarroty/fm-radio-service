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

#include "output_observer.h"

#include "context.h"
#include "operation.h"

#include <QByteArray>
#include <QDebug>
#include <QString>
#include <pulse/pulseaudio.h>

using namespace FmRadioService::audio;

static const char s_defaultSink[] = "sink.primary";

namespace FmRadioService {
namespace audio {

class OutputObserverPrivate
{
    Q_DECLARE_PUBLIC(OutputObserver)

public:
    OutputObserverPrivate(OutputObserver *q);
    ~OutputObserverPrivate();

    void onContextReady();

    void subscriptionCb(pa_subscription_event_type_t t, uint32_t idx);

    void getServerInfoCb(const pa_server_info *info);
    static void get_server_info_cb(pa_context *, const pa_server_info *si,
                                   void *userdata) {
        if (auto thiz = static_cast<OutputObserverPrivate*>(userdata)) {
            thiz->getServerInfoCb(si);
        }
    }

    void getCardInfoList();
    void getCardInfoListCb(const pa_card_info *i, bool isLast);
    static void get_card_info_list_cb(pa_context *, const pa_card_info *i,
                                      int eol, void *userdata) {
        if (auto thiz = static_cast<OutputObserverPrivate*>(userdata)) {
            thiz->getCardInfoListCb(i, eol != 0);
        }
    }

    void updateWiredState();

private:
    QSharedPointer<Context> m_contextHolder;
    pa_context *m_context;
    OutputState m_outputState;
    bool m_wiredOutputIsAvailable;
    Subscription m_subscription;
    Operation m_serverInfoOp;
    Operation m_cardListOp;
    QByteArray m_defaultSinkName;
    QByteArray m_defaultSourceName;
    OutputObserver *q_ptr;
};

}} // namespace

OutputObserverPrivate::OutputObserverPrivate(OutputObserver *q):
    m_contextHolder(Context::instance()),
    m_context(m_contextHolder->get()),
    m_outputState(OutputState::Speaker),
    m_wiredOutputIsAvailable(false),
    q_ptr(q)
{
    m_contextHolder->onReady([this]() { onContextReady(); });
}

OutputObserverPrivate::~OutputObserverPrivate()
{
}

void OutputObserverPrivate::onContextReady()
{
    m_subscription =
        m_contextHolder->subscribe(PA_SUBSCRIPTION_MASK_CARD,
                                   [this](pa_subscription_event_type_t t,
                                          uint32_t idx) {
        subscriptionCb(t, idx);
    });

    m_serverInfoOp =
        pa_context_get_server_info(m_context, get_server_info_cb, this);

    getCardInfoList();
}

void OutputObserverPrivate::subscriptionCb(pa_subscription_event_type_t t,
                                           uint32_t idx)
{
    qDebug() << Q_FUNC_INFO << t << idx;
    if (t & PA_SUBSCRIPTION_EVENT_CARD) {
        getCardInfoList();
    }
}

void OutputObserverPrivate::getServerInfoCb(const pa_server_info *info)
{
    qDebug() << Q_FUNC_INFO;
    m_defaultSinkName = info->default_sink_name;
    m_defaultSourceName = info->default_source_name;
}

void OutputObserverPrivate::getCardInfoList()
{
    m_wiredOutputIsAvailable = false;
    m_cardListOp =
        pa_context_get_card_info_list(m_context, get_card_info_list_cb, this);
}

void OutputObserverPrivate::getCardInfoListCb(const pa_card_info *ci,
                                              bool isLast)
{
    if (isLast) {
        updateWiredState();
        return;
    }
    qDebug() << "Card name" << ci->name;
    for (uint32_t i = 0; i < ci->n_ports; i++) {
        const pa_card_port_info *pi = ci->ports[i];
        qDebug() << "Port" << pi->name << " - " << pi->description;
        /* TODO: figure out if pulseaudio offers a better way to detect wired
         * earsets. */
        QString name = QString::fromUtf8(pi->name);
        if (name.toLower().contains("-wired") &&
            pi->available == PA_PORT_AVAILABLE_YES) {
            m_wiredOutputIsAvailable = true;
        }
    }
}

void OutputObserverPrivate::updateWiredState()
{
    Q_Q(OutputObserver);
    Q_EMIT q->wiredOutputIsAvailableChanged();
}

OutputObserver::OutputObserver(QObject *parent):
    QObject(parent),
    d_ptr(new OutputObserverPrivate(this))
{
}

OutputObserver::~OutputObserver() = default;

bool OutputObserver::wiredOutputIsAvailable() const
{
    Q_D(const OutputObserver);
    return d->m_wiredOutputIsAvailable;
}
