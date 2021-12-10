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

#include "context.h"

#include <QDebug>

using namespace FmRadioService::audio;

static QWeakPointer<Context> s_instance;

Subscription::~Subscription()
{
    QSharedPointer<Context> context = Context::instance();
    if (context && m_id != 0) {
        context->unsubscribe(m_id);
    }
    m_id = 0;
}

Context::Context():
    m_loop(pa_glib_mainloop_new(g_main_context_default())),
    m_context(nullptr),
    m_lastSubscriptionId(0)
{
    pa_proplist *properties = pa_proplist_new();
    if (Q_UNLIKELY(!properties)) return;

    pa_proplist_sets(properties, PA_PROP_MEDIA_ROLE, "multimedia");
    m_context =
        pa_context_new_with_proplist(pa_glib_mainloop_get_api(m_loop),
                                     "fm-radio-service",
                                     properties);
    pa_proplist_free(properties);

    pa_context_set_state_callback(m_context, Context::notification_cb, this);
    pa_context_connect(m_context, nullptr,
                       static_cast<pa_context_flags_t>(PA_CONTEXT_NOAUTOSPAWN |
                                                       PA_CONTEXT_NOFAIL),
                       nullptr);

    pa_context_set_subscribe_callback(m_context, subscription_cb, this);
}

Context::~Context()
{
    pa_context_unref(m_context);
    pa_glib_mainloop_free(m_loop);
}

QSharedPointer<Context> Context::instance()
{
    QSharedPointer<Context> context = s_instance.toStrongRef();
    if (!context) {
        context.reset(new Context);
        s_instance = context;
    }
    return context;
}

void Context::subscription_cb(pa_context *, pa_subscription_event_type_t t,
                              uint32_t idx, void *userdata)
{
    if (auto thiz = static_cast<Context*>(userdata)) {
        thiz->subscriptionCb(t, idx);
    }
}

void Context::subscriptionCb(pa_subscription_event_type_t t, uint32_t idx)
{
    for (const SubscriptionData &sd: m_subscriptions) {
        if (pa_subscription_match_flags(sd.mask, t)) {
            sd.callback(t, idx);
        }
    }
}

Subscription Context::subscribe(pa_subscription_mask_t mask,
                                const SubscribeCb &cb)
{
    m_subscriptions.insert(++m_lastSubscriptionId,
                           { cb, mask });
    return m_lastSubscriptionId;
}

void Context::unsubscribe(int id) {
    m_subscriptions.remove(id);
}

void Context::notification_cb(pa_context *ctxt, void *userdata)
{
    if (auto thiz = static_cast<Context*>(userdata)) {
        switch (pa_context_get_state(ctxt)) {
        case PA_CONTEXT_READY:
            thiz->onContextReady();
            break;
        case PA_CONTEXT_FAILED:
            qWarning() << "Cannot connect to pulseaudio server";
            break;
        default:
            break;
        }
    }
}

void Context::onReady(const ReadyCb &cb)
{
    if (pa_context_get_state(m_context) == PA_CONTEXT_READY) {
        cb();
    } else {
        m_readyCallbacks.append(cb);
    }
}

void Context::onContextReady()
{
    pa_operation_unref(
        pa_context_subscribe(m_context, PA_SUBSCRIPTION_MASK_ALL,
                             nullptr, nullptr));

    const auto callbacks = std::move(m_readyCallbacks);
    for (const ReadyCb &cb: callbacks) {
        cb();
    }
}
