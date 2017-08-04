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

#include "ewig/buffer.hpp"

#include <immer/flex_vector_transient.hpp>
#include <immer/algorithm.hpp>

#include <scelta.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

namespace ewig {

immer::box<std::string> no_file::name = "*unnamed*";
text no_file::content = {};

bool load_in_progress(const buffer& buf)
{
    return std::holds_alternative<loading_file>(buf.from);
}

bool io_in_progress(const buffer& buf)
{
    return
        !std::holds_alternative<existing_file>(buf.from) &&
        !std::holds_alternative<no_file>(buf.from);
}

buffer update_buffer(buffer buf, buffer_action act)
{
    return scelta::match(
        [&] (auto&& act) {
            if (load_in_progress(buf))
                buf.content = act.file.content;
            buf.from = act.file;
            return buf;
        })(act);
}

namespace {

auto load_file_effect(immer::box<std::string> file_name)
{
    constexpr auto progress_report_rate_bytes = 1 << 20;

    return [=] (auto& ctx) {
        ctx.async([=] {
            auto file = std::wifstream{file_name};
            file.exceptions(std::ifstream::badbit);
            auto begp = file.tellg();
            file.seekg(0, std::ios::end);
            auto endp = file.tellg();
            file.seekg(0, std::ios::beg);
            auto progress = loading_file{ file_name, {}, 0, endp - begp };
            auto content = text{}.transient();
            auto ln = std::wstring{};
            auto lastp = begp;
            auto currp = begp;
            // work-around gcc-7 bug
            // https://www.mail-archive.com/gcc-bugs@gcc.gnu.org/msg533664.html
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
            while (std::getline(file, ln)) {
#pragma GCC diagnostic pop
                content.push_back({begin(ln), end(ln)});
                currp += ln.size();
                if (currp - lastp > progress_report_rate_bytes) {
                    progress.content = content.persistent();
                    progress.loaded_bytes = currp - begp;
                    ctx.dispatch(load_progress_action{progress});
                    lastp = currp;
                }
            }
            ctx.dispatch(load_done_action{{file_name, content.persistent()}});
        });
    };
}

auto save_file_effect(immer::box<std::string> file_name, text content)
{
    constexpr auto progress_report_rate_lines = (1 << 20) / 40;

    return [=] (auto& ctx) {
        ctx.async([=] {
            using namespace std::chrono;
            auto file = std::wofstream{file_name};
            file.exceptions(std::ifstream::badbit);
            auto lastp = std::size_t{};
            auto progress = saving_file{ file_name, content, 0 };
            immer::for_each(content, [&] (auto l) {
                immer::for_each_chunk(l, [&] (auto first, auto last) {
                    file.write(first, last - first);
                });
                file.put('\n');
                ++progress.saved_lines;
                if (progress.saved_lines - lastp > progress_report_rate_lines) {
                    ctx.dispatch(save_progress_action{progress});
                    lastp = progress.saved_lines;
                }
            });
            ctx.dispatch(save_done_action{{file_name, content}});
        });
    };
}

} // anonymous

result<buffer, buffer_action> save_buffer(buffer buf)
{
    auto file = std::get<existing_file>(buf.from);
    buf.from = saving_file{file.name, buf.content, {}};
    return { buf, save_file_effect(file.name, buf.content) };
}

result<buffer, buffer_action> load_buffer(buffer buf, const std::string& fname)
{
    buf.from = loading_file{fname, {}, {}, 1};
    return { buf, load_file_effect(fname) };
}

bool is_dirty(const buffer& buf)
{
    return scelta::match(
        [&](auto&& x) { return buf.content != x.content; })
        (buf.from);
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

coord actual_display_cursor(const buffer& buf)
{
    auto cursor = actual_cursor(buf);
    if (cursor.row < (index)buf.content.size())
        cursor.col = display_line_col(buf.content[cursor.row], cursor.col);
    return cursor;
}

buffer page_up(buffer buf, coord size)
{
    if (buf.scroll.row > size.row) {
        buf.scroll.row -= size.row;
        if (buf.cursor.row >= buf.scroll.row + size.row)
            buf.cursor.row = buf.scroll.row + size.row - 2;
    } else if (buf.scroll.row > 0) {
        buf.scroll.row = 0;
        if (buf.cursor.row >= size.row)
            buf.cursor.row = size.row - 2;
    } else {
        buf.cursor.row = 0;
    }
    return buf;
}

buffer page_down(buffer buf, coord size)
{
    if (buf.scroll.row + size.row < (index)buf.content.size()) {
        buf.scroll.row += size.row;
        if (buf.cursor.row < buf.scroll.row)
            buf.cursor.row = buf.scroll.row + 1;
    } else {
        buf.cursor.row = buf.content.size();
    }
    return buf;
}

buffer move_cursor_up(buffer buf)
{
    buf.cursor.row = std::max(buf.cursor.row - 1, 0);
    return buf;
}

buffer move_cursor_down(buffer buf)
{
    buf.cursor.row = std::min(buf.cursor.row + 1, (index)buf.content.size());
    return buf;
}

buffer move_line_start(buffer buf)
{
    buf.cursor.col = 0;
    return buf;
}

buffer move_line_end(buffer buf)
{
    if (buf.cursor.row < (index)buf.content.size())
        buf.cursor.col = buf.content[buf.cursor.row].size();
    return buf;
}

buffer move_buffer_start(buffer buf)
{
    buf.cursor = {0,0};
    return buf;
}

buffer move_buffer_end(buffer buf)
{
    buf.cursor = {(index)buf.content.size(),0};
    return buf;
}

buffer move_cursor_left(buffer buf)
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

buffer move_cursor_right(buffer buf)
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

buffer scroll_to_cursor(buffer buf, coord wsize)
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

buffer insert_new_line(buffer buf)
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

buffer insert_tab(buffer buf)
{
    return insert_char(buf, '\t');
}

buffer insert_char(buffer buf, wchar_t value)
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

buffer delete_char(buffer buf)
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

buffer delete_char_right(buffer buf)
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

std::pair<buffer, text> cut_rest(buffer buf)
{
    if (buf.cursor.row < (index)buf.content.size()) {
        auto ln = buf.content[buf.cursor.row];
        if (buf.cursor.col < (index)ln.size()) {
            buf.content = buf.content.set(buf.cursor.row, ln.take(buf.cursor.col));
            return {buf, {ln.drop(buf.cursor.col)}};
        } else {
            // Delete the end of line to join with previous line
            return {delete_char_right(buf), {{}, {}}};
        }
    } else {
        return {buf, {}};
    }
}

buffer insert_text(buffer buf, text paste)
{
    auto cur = actual_cursor(buf);
    if (cur.row < (index)buf.content.size()) {
        auto ln1 = buf.content[cur.row];
        buf.content = buf.content.set(cur.row,
                                      ln1.take(cur.col) + paste[0]);
        buf.content = buf.content.take(cur.row + 1)
            + paste.drop(1)
            + buf.content.drop(cur.row + 1);
        auto ln2 = buf.content[cur.row + paste.size() - 1];
        buf.content = buf.content.set(cur.row + paste.size() - 1,
                                      ln2 + ln1.drop(cur.col));
    } else {
        buf.content = buf.content + paste;
    }
    buf.cursor.row = cur.row + paste.size() - 1;
    buf.cursor.col = paste.size() > 1
        ? paste.back().size()
        : cur.col + paste.back().size();
    return buf;
}

text selected_text(buffer buf)
{
    auto [starts, ends] = selected_region(buf);
    if (starts == ends)
        return {};
    else
        return // actually add the imaginary line if the selection
               // ends there
            (ends.row == (index)buf.content.size()
                ? buf.content.push_back({})
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

std::pair<buffer, text> cut(buffer buf)
{
    auto selection = selected_text(buf);
    auto [starts, ends] = selected_region(buf);
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

    buf.selection_start = std::nullopt;
    return {buf, selection};
}

std::pair<buffer, text> copy(buffer buf)
{
    auto result = selected_text(buf);
    buf.selection_start = std::nullopt;
    return {buf, result};
}

buffer start_selection(buffer buf)
{
    auto cur = actual_cursor(buf);
    buf.selection_start = cur;
    return buf;
}

buffer select_whole_buffer(buffer buf)
{
    buf.cursor = {0, 0};
    buf.selection_start = {(index)buf.content.size(), (index)buf.content.back().size()};
    return buf;
}

buffer clear_selection(buffer buf)
{
    buf.selection_start = std::nullopt;
    return buf;
}

std::tuple<coord, coord> selected_region(buffer buf)
{
    if (buf.selection_start) {
        auto cursor = actual_cursor(buf);
        auto starts = std::min(cursor, *buf.selection_start);
        auto ends   = std::max(cursor, *buf.selection_start);
        starts.col = starts.row < (index)buf.content.size()
                                  ? display_line_col(buf.content[starts.row], starts.col) : 0;
        ends.col   = ends.row < (index)buf.content.size()
                                ? display_line_col(buf.content[ends.row], ends.col) : 0;
        return {starts, ends};
    } else {
        return {};
    }
}

buffer undo(buffer buf)
{
    auto idx = buf.history_pos.value_or(buf.history.size());
    if (idx > 0) {
        auto restore = buf.history[--idx];
        buf.content = restore.content;
        buf.cursor = restore.cursor;
        buf.history_pos = idx;
    }
    return buf;
}

std::pair<buffer, std::string> record(buffer before, buffer after)
{
    if (before.content != after.content) {
        if (load_in_progress(before)) {
            return {before, "can't edit while loading"};
        } else {
            after.history = after.history.push_back({before.content, before.cursor});
            if (before.history_pos == after.history_pos)
                after.history_pos = std::nullopt;
        }
    }
    return {after, ""};
}

} // namespace ewig
