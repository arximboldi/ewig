//
// ewig - an immutable text editor
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of ewig.
//
// ewig is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ewig is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ewig.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <ewig/keys.hpp>
#include <ewig/buffer.hpp>

#include <lager/store.hpp>

#include <ctime>
#include <variant>
#include <any>

namespace ewig {

struct key_action { key_code key; };
struct resize_action { coord size; };

struct command_action
{
    immer::box<std::string> name;
    std::any arg;
};

using action = std::variant<command_action,
                           key_action,
                           resize_action,
                           buffer_action>;

struct message
{
    std::time_t time_stamp;
    immer::box<std::string> content;
};

struct application
{
    coord window_size;
    key_map keys;
    key_seq input;
    buffer current;
    immer::vector<text> clipboard;
    immer::vector<message> messages;
};

using command = std::function<
    std::pair<application, lager::effect<action>>(
        application, std::any)>;

coord editor_size(application app);

application paste(application app, coord size);
application put_message(application state, immer::box<std::string> str);
application put_clipboard(application state, text content);
application clear_input(application state);

std::pair<application, lager::effect<action>> quit(application app);
std::pair<application, lager::effect<action>> save(application app);
std::pair<application, lager::effect<action>> load(application app, const std::string& fname);
std::pair<application, lager::effect<action>> update(application state, action ev);

application apply_edit(application state, coord size, buffer edit);
application apply_edit(application state, coord size, std::pair<buffer, text> edit);

} // namespace ewig
