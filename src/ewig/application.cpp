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

#include <scelta.hpp>

using namespace std::string_literals;

namespace ewig {

namespace {

template <typename T>
struct arg
{
    template <typename Fn, typename... Args>
    static auto invoke(Fn&& fn, const std::any& arg, Args&&... args)
    {
        return std::forward<Fn>(fn)(std::forward<Args>(args)...,
                                   std::any_cast<T>(arg));
    }
};

template <>
struct arg<void>
{
    template <typename Fn, typename... Args>
    static auto invoke(Fn&& fn, const std::any&, Args&&... args)
    {
        return std::forward<Fn>(fn)(std::forward<Args>(args)...);
    }
};

template <typename Arg=void, typename Fn>
command app_command(Fn fn)
{
    return [=] (application state, std::any x) {
        return arg<Arg>::invoke(fn, x, state);
    };
}

template <typename Arg=void, typename Fn>
command edit_command(Fn fn)
{
    return [=] (application state, std::any x) {
        return apply_edit(state, arg<Arg>::invoke(fn, x, state.current));
    };
}

template <typename Arg=void, typename Fn>
command paste_command(Fn fn)
{
    return [=] (application state, std::any x) {
        return apply_edit(state, arg<Arg>::invoke(fn, x,
                                                 state.current,
                                                 state.clipboard.back()));
    };
}

template <typename Arg=arg<void>, typename Fn>
command scroll_command(Fn fn)
{
    return [=] (application state, std::any arg) {
        state.current = Arg::invoke(fn, arg,
                                   state.current,
                                   editor_size(state));
        return state;
    };
}

} // anonymous namespace

using commands = std::unordered_map<std::string, command>;

static const auto global_commands = commands
{
    {"insert",                 edit_command<wchar_t>(insert_char)},
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
    {"quit",                   app_command(quit)},
    {"save",                   app_command(save)},
    {"load",                   app_command<std::string>(load)},
    {"message",                app_command<std::string>(put_message)},
    {"undo",                   edit_command(undo)},
    {"start-selection",        edit_command(start_selection)},
    {"select-whole-buffer",    edit_command(select_whole_buffer)},
};

result<application, action> quit(application app)
{
    return {
        put_message(app, "quitting... (waiting for operations to finish)"),
        [] (auto&& ctx) { ctx.finish(); }
    };
}

result<application, action> save(application state)
{
    if (!is_dirty(state.current)) {
        return put_message(state, "nothing to save");
    } else if (io_in_progress(state.current)) {
        return put_message(state, "can't save while saving or loading the file");
    } else {
        auto [buffer, effect] = save_buffer(state.current);
        state.current = buffer;
        return {state, effect};
    }
}

result<application, action> load(application state, const std::string& fname)
{
    if (io_in_progress(state.current)) {
        return put_message(state, "can't load while saving or loading the file");
    } else {
        auto [buffer, effect] = load_buffer(state.current, fname);
        state.current = buffer;
        return {state, effect};
    }
}

application put_message(application state, immer::box<std::string> str)
{
    if (!str->empty()) {
        state.messages = std::move(state.messages)
            .push_back({std::time(nullptr), std::move(str)});
    }
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

coord editor_size(application app)
{
    return {app.window_size.row - 2, app.window_size.col};
}

application clear_input(application state)
{
    state.input = {};
    return state;
}

result<application, action> update(application state, action ev)
{
    using result_t = result<application, action>;

    return scelta::match(
        [&](const command_action& ev) -> result_t
        {
            auto it = global_commands.find(ev.name);
            if (it != global_commands.end()) {
                state = put_message(state, "calling command: "s + *ev.name);
                return it->second(state, ev.arg);
            } else {
                return put_message(state, "unknown command: "s + *ev.name);
            }
        },
        [&](const buffer_action& ev) -> result_t
        {
            auto [buffer, msg] = update_buffer(state.current, ev);
            state.current = buffer;
            return put_message(state, msg);
        },
        [&](const resize_action& ev) -> result_t
        {
            state.window_size = ev.size;
            return state;
        },
        [&](const key_action& ev) -> result_t
        {
            state.input = state.input.push_back(ev.key);
            const auto& map = state.keys.get();
            auto it = map.find(state.input);
            if (it != map.end()) {
                if (!it->second->empty()) {
                    auto result = update(state, command_action{it->second, {}});
                    return {clear_input(result.first), result.second};
                }
            } else if (key_seq{ev.key} != key::ctrl('[')) {
                using std::get;
                auto is_single_char = state.input.size() == 1;
                auto [kres, kkey] = ev.key;
                if (is_single_char && !kres && !std::iscntrl(kkey)) {
                    auto result = update(
                        state, command_action{"insert", (wchar_t)kkey});
                    return {clear_input(result.first), result.second};
                } else {
                    return clear_input(put_message(state, "unbound key sequence: " +
                                                   to_string(state.input)));
                }
            }
            return state;
        })(ev);
}

application apply_edit(application state, buffer edit)
{
    auto msg = std::string{};
    std::tie(state.current, msg) =
        record(state.current, scroll_to_cursor(edit, editor_size(state)));
    return put_message(state, msg);
}

application apply_edit(application state, std::pair<buffer, text> edit)
{
    auto msg = std::string{};
    std::tie(state.current, msg) =
        record(state.current, scroll_to_cursor(edit.first, editor_size(state)));
    return put_message(put_clipboard(state, edit.second), msg);
}

application put_clipboard(application state, text content)
{
    if (!content.empty())
        state.clipboard = state.clipboard.push_back(content);
    return state;
}

} // namespace ewig
