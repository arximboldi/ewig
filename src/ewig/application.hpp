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

#include <ctime>

namespace ewig {

struct message
{
    std::time_t time_stamp;
    immer::box<std::string> content;
};

struct application
{
    buffer current;
    key_map keys;
    key_seq input;
    immer::vector<text> clipboard;
    immer::vector<message> messages;
};

using command = std::function<boost::optional<application>(application, coord)>;

application save(application app, coord);
application paste(application app, coord size);
application put_message(application state, std::string str);
application put_clipboard(application state, text content);

boost::optional<application> eval_command(application state, const std::string& cmd, coord editor_size);
application eval_insert_char(application state, wchar_t key, coord editor_size);

application clear_input(application state);
boost::optional<application> handle_key(application state, key_code key, coord size);

application apply_edit(application state, coord size, buffer edit);
application apply_edit(application state, coord size, text edit);
application apply_edit(application state, coord size, std::pair<buffer, text> edit);

template <typename Fn>
command edit_command(Fn fn)
{
    return [=] (application state, coord size) {
        return apply_edit(state, size, fn(state.current));
    };
}

template <typename Fn>
command paste_command(Fn fn)
{
    return [=] (application state, coord size) {
        return apply_edit(state, size, fn(state.current, state.clipboard.back()));
    };
}

template <typename Fn>
command scroll_command(Fn fn)
{
    return [=] (application state, coord wsize) {
        state.current = fn(state.current, wsize);
        return state;
    };
}

template <typename Fn>
command key_command(Fn fn)
{
    return [=] (application state, coord size) {
        auto key = std::get<1>(state.input.back());
        return apply_edit(state, size, fn(state.current, key));
    };
}

} // namespace ewig
