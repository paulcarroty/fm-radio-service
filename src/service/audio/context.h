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

#ifndef FM_RADIO_SERVICE_AUDIO_CONTEXT_H
#define FM_RADIO_SERVICE_AUDIO_CONTEXT_H

#include <QHash>
#include <QSharedPointer>
#include <QVector>
#include <functional>
#include <pulse/glib-mainloop.h>
#include <pulse/pulseaudio.h>

namespace FmRadioService {
namespace audio {

class Subscription
{
public:
    Subscription(int id = 0): m_id(id) {}
    ~Subscription();

    Subscription(Subscription &&other) noexcept:
        m_id(other.m_id) { other.m_id = 0; }
    Subscription &operator=(Subscription &&other) {
        std::swap(m_id, other.m_id);
        return *this;
    }

private:
    int m_id;
};

class Context
{
public:
    ~Context();

    static QSharedPointer<Context> instance();

    using ReadyCb = std::function<void()>;
    void onReady(const ReadyCb &cb);

    pa_context *get() const { return m_context; }

    using SubscribeCb =
        std::function<void(pa_subscription_event_type_t, uint32_t)>;
    Subscription subscribe(pa_subscription_mask_t, const SubscribeCb &cb);

private:
    Context();
    static void subscription_cb(pa_context *, pa_subscription_event_type_t t,
                                uint32_t idx, void *userdata);
    void subscriptionCb(pa_subscription_event_type_t t, uint32_t idx);
    static void notification_cb(pa_context *ctxt, void *userdata);
    void onContextReady();

    friend class Subscription;
    void unsubscribe(int id);

    struct SubscriptionData {
        SubscribeCb callback;
        pa_subscription_mask_t mask;
    };

private:
    pa_glib_mainloop *m_loop;
    pa_context *m_context;
    QVector<ReadyCb> m_readyCallbacks;
    QHash<int, SubscriptionData> m_subscriptions;
    int m_lastSubscriptionId;
};

} // namespace
} // namespace

#endif // FM_RADIO_SERVICE_AUDIO_CONTEXT_H
