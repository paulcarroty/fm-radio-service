/*
 * Copyright © 2021 UBports Foundation.
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

#ifndef FM_RADIO_SERVICE_ERROR_H
#define FM_RADIO_SERVICE_ERROR_H

#include <QDebug>
#include <QMetaType>
#include <QString>

namespace FmRadioService {

class Error
{
public:
    enum Code {
        NoError,
        ResourceError,
        ResourceBusy,
        TunerError,  // some tuner command failed
        AudioError,
    };

    Error(Code code = NoError, const QString &message = QString()):
        m_code(code), m_message(message) {}
    Error(const Error &e) = default;

    bool operator==(const Error &o) const {
        return o.code() == code() && o.message() == message();
    }

    bool operator!=(const Error &o) const {
        return o.code() != code() || o.message() != message();
    }

    Code code() const { return m_code; }
    bool isError() const { return m_code != NoError; }
    explicit operator bool() const { return isError(); }

    const QString &message() const { return m_message; }

    QString toString() const {
        return m_message + QString(" (%1)").arg(m_code);
    }

private:
    Code m_code;
    QString m_message;
};

inline QDebug operator<<(QDebug dbg, const Error &error)
{
    dbg.nospace() << "Error(" << error.code() << "): " << error.message();
    return dbg.maybeSpace();
}

} // namespace

Q_DECLARE_METATYPE(FmRadioService::Error)

#endif // FM_RADIO_SERVICE_ERROR_H
