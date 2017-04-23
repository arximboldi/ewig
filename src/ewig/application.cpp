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

#include "ewig/application.hpp"

using namespace std::string_literals;

namespace ewig {

using commands = std::unordered_map<std::string, command>;

static const auto global_commands = commands
{
    {"insert",                 key_command(insert_char)},
    {"delete-char",            edit_command(delete_char)},
    {"delete-char-right",      edit_command(delete_char_right)},
    {"insert-tab",             edit_command(insert_tab)},
    {"kill-line",              edit_command(cut_rest)},
    {"copy",                   edit_command(copy)},
    {"cut",                    edit_command(cut)},
    {"move-beginning-of-line", edit_command(move_line_start)},
    {"move-beginning-buffer",  edit_command(move_buffer_start)},
    {"move-end-buffer",        edit_command(move_buffer_end)},
    {"move-down",              edit_command(move_cursor_down)},
    {"move-end-of-line",       edit_command(move_line_end)},
    {"move-left",              edit_command(move_cursor_left)},
    {"move-right",             edit_command(move_cursor_right)},
    {"move-up",                edit_command(move_cursor_up)},
    {"new-line",               edit_command(insert_new_line)},
    {"page-down",              scroll_command(page_down)},
    {"page-up",                scroll_command(page_up)},
    {"paste",                  paste_command(insert_text)},
    {"quit",                   [](auto, auto) { return boost::none; }},
    {"start-selection",        edit_command(start_selection)},
    {"select-whole-buffer",    edit_command(select_whole_buffer)},
};

application put_message(application state, std::string str)
{
    state.messages = std::move(state.messages)
        .push_back({std::time(nullptr), std::move(str)});
    return state;
}

coord actual_cursor(buffer buf)
{
    return {
        buf.cursor.row,
        std::min(buf.cursor.row < (index)buf.content.size()
                 ? (index)buf.content[buf.cursor.row].size() : 0,
                 buf.cursor.col)
    };
}

coord editor_size(coord size)
{
    return {size.row - 2, size.col};
}

application clear_input(application state)
{
    state.input = {};
    return state;
}

boost::optional<application> handle_key(application state, key_code key, coord size)
{
    state.input = state.input.push_back(key);
    const auto& map = state.keys.get();
    auto it = map.find(state.input);
    if (it != map.end()) {
        if (!it->second.empty()) {
            auto result = eval_command(state, it->second, size);
            return optional_map(result, clear_input);
        }
    } else if (key_seq{key} != key::ctrl('[')) {
        using std::get;
        auto is_single_char = state.input.size() == 1;
        if (is_single_char && get<0>(key) == OK && !std::iscntrl(get<1>(key))) {
            auto result = eval_command(state, "insert", size);
            return optional_map(result, clear_input);
        } else {
            return clear_input(put_message(state, "unbound key sequence"));
        }
    }
    return state;
}

boost::optional<application>
eval_command(application state, const std::string& cmd, coord size)
{
    auto it = global_commands.find(cmd);
    if (it != global_commands.end()) {
        return it->second(put_message(state, "calling command: "s + cmd), size);
    } else {
        return put_message(state, "unknown_command: "s + cmd);
    }
}

application apply_edit(application state, coord size, buffer edit)
{
    state.buffer = scroll_to_cursor(edit, editor_size(size));
    return state;
}

application apply_edit(application state, coord size, std::pair<buffer, text> edit)
{
    state.buffer = scroll_to_cursor(edit.first, editor_size(size));
    return put_clipboard(state, edit.second);
}

application put_clipboard(application state, text content)
{
    if (!content.empty())
        state.clipboard = state.clipboard.push_back(content);
    return state;
}

} // namespace ewig
