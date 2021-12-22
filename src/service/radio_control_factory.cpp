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

#include "radio_control_factory.h"

#include "abstract_radio_control.h"
#include "cherokee/radio_control.h"
#include "mediatek/radio_control.h"

#include <QVector>

using namespace FmRadioService;

AbstractRadioControl *RadioControlFactory::createRadioControl()
{
    AbstractRadioControl *control = nullptr;

    using Constructor = std::function<AbstractRadioControl*()>;
    const QVector<Constructor> backends = {
        []() { return new Cherokee::RadioControl(); },
        []() { return new Mediatek::RadioControl(); },
    };
    for (const auto backend: backends) {
        control = backend();
        if (control->checkHardware()) {
            return control;
        }
        delete control;
    }
    return nullptr;
}
