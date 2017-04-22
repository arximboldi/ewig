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

#include "ewig/model.hpp"

#include <immer/flex_vector_transient.hpp>
#include <immer/algorithm.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std::string_literals;

namespace ewig {

using commands = std::unordered_map<std::string, command>;

static const auto global_commands = commands
{
    {"delete-char",            edit_command(delete_char)},
    {"delete-char-right",      edit_command(delete_char_right)},
    {"insert-tab",             edit_command(insert_tab)},
    {"kill-line",              edit_command(delete_rest)},
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
    {"paste",                  edit_command(paste)},
    {"quit",                   [](auto, auto) { return boost::none; }},
    {"start-selection",        edit_command(start_selection)},
    {"select-whole-buffer",    edit_command(select_whole_buffer)},
};

boost::optional<app_state>
eval_command(app_state state, const std::string& cmd, coord editor_size)
{
    auto it = global_commands.find(cmd);
    if (it != global_commands.end()) {
        return it->second(put_message(state, "calling command: "s + cmd),
                          editor_size);
    } else {
        return put_message(state, "unknown_command: "s + cmd);
    }
}

app_state eval_insert_char(app_state state, wchar_t key, coord editor_size)
{
    state.buffer = scroll_to_cursor(
        insert_char(clear_selection(state.buffer), key),
        editor_size);
    return state;
}

file_buffer load_file(const char* file_name)
{
    auto file = std::wifstream{file_name};
    auto content = text{}.transient();
    file.exceptions(std::ifstream::badbit);
    auto ln = std::wstring{};
    while (std::getline(file, ln))
        content.push_back({begin(ln), end(ln)});
    auto result = content.persistent();
    return { result, {}, {}, {}, file_name, result };
}

app_state put_message(app_state state, string_t str)
{
    state.messages = std::move(state.messages)
        .push_back({std::time(nullptr), str});
    return state;
}

coord actual_cursor(file_buffer buf)
{
    return {
        buf.cursor.row,
        std::min(buf.cursor.row < (index)buf.content.size()
                 ? (index)buf.content[buf.cursor.row].size() : 0,
                 buf.cursor.col)
    };
}

index display_line_col(const line& ln, index col)
{
    using namespace std;
    auto cur_col = index{};
    immer::for_each(begin(ln), begin(ln) + col, [&] (auto c) {
        if (c == '\t') {
            cur_col += tab_width - (cur_col % tab_width);
        } else
            ++cur_col;
    });
    return cur_col;
}

coord actual_display_cursor(const file_buffer& buf)
{
    auto cursor = actual_cursor(buf);
    if (cursor.row < (index)buf.content.size())
        cursor.col = display_line_col(buf.content[cursor.row], cursor.col);
    return cursor;
}

file_buffer page_up(file_buffer buf, coord size)
{
    if (buf.scroll.row > size.row) {
        buf.scroll.row -= size.row;
        if (buf.cursor.row >= buf.scroll.row + size.row)
            buf.cursor.row = buf.scroll.row + size.row - 1;
    } else if (buf.scroll.row > 0) {
        buf.scroll.row = 0;
        if (buf.cursor.row >= size.row)
            buf.cursor.row = size.row - 1;
    } else {
        buf.cursor.row = 0;
    }
    return buf;
}

file_buffer page_down(file_buffer buf, coord size)
{
    if (buf.scroll.row + size.row < (index)buf.content.size()) {
        buf.scroll.row += size.row;
        if (buf.cursor.row < buf.scroll.row)
            buf.cursor.row = buf.scroll.row;
    } else {
        buf.cursor.row = buf.content.size();
    }
    return buf;
}

file_buffer move_cursor_up(file_buffer buf)
{
    buf.cursor.row = std::max(buf.cursor.row - 1, 0);
    return buf;
}

file_buffer move_cursor_down(file_buffer buf)
{
    buf.cursor.row = std::min(buf.cursor.row + 1, (index)buf.content.size());
    return buf;
}

file_buffer move_line_start(file_buffer buf)
{
    buf.cursor.col = 0;
    return buf;
}

file_buffer move_line_end(file_buffer buf)
{
    if (buf.cursor.row < (index)buf.content.size())
        buf.cursor.col = buf.content[buf.cursor.row].size();
    return buf;
}

file_buffer move_buffer_start(file_buffer buf)
{
    buf.cursor = {0,0};
    return buf;
}

file_buffer move_buffer_end(file_buffer buf)
{
    buf.cursor = {(index)buf.content.size(),0};
    return buf;
}

file_buffer move_cursor_left(file_buffer buf)
{
    auto cur = actual_cursor(buf);
    if (cur.col - 1 < 0) {
        if (cur.row > 0) {
            buf.cursor.row -= 1;
            buf.cursor.col = buf.cursor.row < (index)buf.content.size()
                           ? (index)buf.content[buf.cursor.row].size() : 0;
        }
    } else  {
        buf.cursor.col = std::max(0, cur.col - 1);
    }
    return buf;
}

file_buffer move_cursor_right(file_buffer buf)
{
    auto cur = actual_cursor(buf);
    auto max = buf.cursor.row < (index)buf.content.size()
             ? (index)buf.content[buf.cursor.row].size() : 0;
    if (cur.col + 1 > max) {
        buf = move_cursor_down(buf);
        buf.cursor.col = 0;
    } else {
        buf.cursor.col = std::max(0, buf.cursor.col + 1);
    }
    return buf;
}

file_buffer scroll_to_cursor(file_buffer buf, coord wsize)
{
    auto cur = actual_display_cursor(buf);
    if (cur.row >= wsize.row + buf.scroll.row) {
        buf.scroll.row = cur.row - wsize.row + 1;
    } else if (cur.row < buf.scroll.row) {
        buf.scroll.row = cur.row;
    }
    if (cur.col >= wsize.col + buf.scroll.col) {
        buf.scroll.col = cur.col - wsize.col + 1;
    } else if (cur.col < buf.scroll.col) {
        buf.scroll.col = cur.col;
    }
    return buf;
}

file_buffer insert_new_line(file_buffer buf)
{
    auto cur = actual_cursor(buf);
    if (cur.row == (index)buf.content.size()) {
        buf.content = buf.content.push_back({});
        return move_cursor_down(buf);
    } else {
        auto ln = buf.content[cur.row];
        if (cur.row + 1 < (index)buf.content.size() ||
            cur.col + 1 < (index)ln.size()) {
            buf.content = buf.content
                .set(cur.row, ln.take(cur.col))
                .insert(cur.row + 1, ln.drop(cur.col));
        }
        buf = move_cursor_down(buf);
        buf.cursor.col = 0;
        return buf;
    }
}

file_buffer insert_tab(file_buffer buf)
{
    return insert_char(buf, '\t');
}

file_buffer insert_char(file_buffer buf, wchar_t value)
{
    auto cur = actual_cursor(buf);
    if (cur.row == (index)buf.content.size()) {
        buf.content = buf.content.push_back({value});
    } else {
        buf.content = buf.content.update(cur.row, [&] (auto l) {
            return l.insert(cur.col, value);
        });
    }
    buf.cursor.col = cur.col + 1;
    return buf;
}

file_buffer delete_char(file_buffer buf)
{
    auto cur = actual_cursor(buf);

    if (cur.col >= 1) {
        buf.content = buf.content.update(cur.row, [&] (auto l) {
            return l.erase(cur.col - 1);
        });
        buf.cursor.col = cur.col - 1;
    } else if (cur.col == 0 && cur.row > 0) {
        auto ln1 = buf.content[cur.row - 1];
        if (cur.row < (index)buf.content.size()) {
            buf.content = buf.content
                .update(cur.row, [&] (auto ln2) { return ln1 + ln2; })
                .erase(cur.row - 1);
        }
        buf.cursor.row --;
        buf.cursor.col = ln1.size();
    }
    return buf;
}

file_buffer delete_char_right(file_buffer buf)
{
    auto cur = actual_cursor(buf);

    if (cur.col < (index)buf.content[cur.row].size()) {
        buf.content = buf.content.update(cur.row, [&] (auto l) {
            return l.erase(cur.col);
        });
    } else if (cur.row + 1 < (index)buf.content.size()) {
        auto ln2 = buf.content[cur.row + 1];
        buf.content = buf.content
            .update(cur.row, [&] (auto ln1) { return ln1 + ln2; })
            .erase(cur.row + 1);
    }
    return buf;
}

file_buffer delete_rest(file_buffer buf)
{
    if (buf.cursor.row < (index)buf.content.size()) {
        auto ln = buf.content[buf.cursor.row];
        if (buf.cursor.col < (index)ln.size()) {
            buf.content = buf.content.set(buf.cursor.row, ln.take(buf.cursor.col));
            buf.clipboard = buf.clipboard.push_back({ln.drop(buf.cursor.col)});
            return buf;
        } else {
            // Delete the end of line to join with previous line
            buf.clipboard = buf.clipboard.push_back({{}, {}});
            return delete_char_right(buf);
        }
    } else {
        return buf;
    }
}

file_buffer paste(file_buffer buf)
{
    auto cur = actual_cursor(buf);

    if (!buf.clipboard.empty()) {
        auto to_copy = buf.clipboard.back();
        if (cur.row < (index)buf.content.size()) {
            auto ln1 = buf.content[cur.row];
            buf.content = buf.content.set(cur.row,
                                          ln1.take(cur.col) + to_copy[0]);
            buf.content = buf.content.take(cur.row + 1)
                + to_copy.drop(1)
                + buf.content.drop(cur.row + 1);
            auto ln2 = buf.content[cur.row + to_copy.size() - 1];
            buf.content = buf.content.set(cur.row + to_copy.size() - 1,
                                          ln2 + ln1.drop(cur.col));
        } else {
            buf.content = buf.content + to_copy;
        }
        buf.cursor.row = cur.row + to_copy.size() - 1;
        buf.cursor.col = to_copy.size() > 1
            ? to_copy.back().size()
            : cur.col + to_copy.back().size();
    }

    return buf;
}

text selected_text(file_buffer buf)
{
    coord starts, ends;
    std::tie(starts, ends) = selected_region(buf);
    return (ends.row == (index)buf.content.size()
            ? buf.content.push_back({}) // actually add the imaginary
                                        // line if the selection ends
                                        // there
            : buf.content)
               .take(ends.row+1)
               .drop(starts.row)
               .update(ends.row-starts.row, [&] (auto l) {
                   return l.take(ends.col);
               })
               .update(0, [&] (auto l) {
                   return l.drop(starts.col);
               });
}

file_buffer cut(file_buffer buf)
{
    buf.clipboard = buf.clipboard.push_back(selected_text(buf));
    coord starts, ends;
    std::tie(starts, ends) = selected_region(buf);
    if (starts != ends) {
        if (starts.row != ends.row) {
            auto content =
                ends.row == (index)buf.content.size()
                ? buf.content.push_back({}) // add the imaginary line
                : buf.content;
            buf.content = content
                .take(starts.row+1)
                .update(starts.row, [&] (auto l) {
                    return l.take(starts.col) + content[ends.row].drop(ends.col);
                })
              + buf.content
                .drop(ends.row+1);
        } else {
            buf.content = buf.content.update(starts.row, [&] (auto l) {
                return l.take(starts.col) + l.drop(ends.col);
            });
        }

        buf.cursor = starts;
    }

    buf.selection_start = boost::none;
    return buf;
}

file_buffer copy(file_buffer buf)
{
    coord starts, ends;
    std::tie(starts, ends) = selected_region(buf);
    if (starts != ends) {
        buf.clipboard = buf.clipboard.push_back(selected_text(buf));
    }
    buf.selection_start = boost::none;
    return buf;
}

file_buffer start_selection(file_buffer buf)
{
    auto cur = actual_cursor(buf);
    buf.selection_start = cur;
    return buf;
}

file_buffer select_whole_buffer(file_buffer buf)
{
    buf.cursor = {0, 0};
    buf.selection_start = {(index)buf.content.size(), (index)buf.content.back().size()};
    return buf;
}

file_buffer clear_selection(file_buffer buf)
{
    buf.selection_start = boost::none;
    return buf;
}

std::tuple<coord, coord> selected_region(file_buffer buf)
{
    auto cursor = actual_cursor(buf);
    auto starts = std::min(cursor, *buf.selection_start);
    auto ends   = std::max(cursor, *buf.selection_start);
    starts.col = starts.row < (index)buf.content.size()
               ? display_line_col(buf.content[starts.row], starts.col) : 0;
    ends.col   = ends.row < (index)buf.content.size()
               ? display_line_col(buf.content[ends.row], ends.col) : 0;
    return {starts, ends};
}

} // namespace ewig
