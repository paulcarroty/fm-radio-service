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

#ifndef FM_RADIO_SERVICE_AUDIO_OPERATION_H
#define FM_RADIO_SERVICE_AUDIO_OPERATION_H

#include <pulse/pulseaudio.h>

namespace FmRadioService {
namespace audio {

class Operation
{
public:
    Operation(pa_operation *op = nullptr): m_op(op) {}
    Operation(const Operation &other):
        m_op(pa_operation_ref(other.m_op)) {}
    ~Operation() {
        if (m_op) pa_operation_unref(m_op);
    }

    Operation &operator=(const Operation &other) {
        if (m_op) pa_operation_unref(m_op);
        m_op = other.m_op ? pa_operation_ref(other.m_op) : nullptr;
        return *this;
    }

    bool isValid() const { return m_op != nullptr; }
    bool isRunning() const { return state() == PA_OPERATION_RUNNING; }
    pa_operation_state_t state() const {
        return m_op ? pa_operation_get_state(m_op) : PA_OPERATION_CANCELLED;
    }

    void cancel() {
        if (m_op) pa_operation_cancel(m_op);
    }

private:
    pa_operation *m_op;
};

} // namespace
} // namespace

#endif // FM_RADIO_SERVICE_AUDIO_OPERATION_H
