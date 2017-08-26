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

std::pair<buffer, std::string> update_buffer(buffer buf, buffer_action act)
{
    using namespace std::string_literals;

    return scelta::match(
        [&] (load_progress_action& act) {
            buf.content = act.file.content;
            buf.from = act.file;
            return std::pair{buf, ""s};
        },
        [&] (load_done_action& act) {
            buf.content = act.file.content;
            buf.from = act.file;
            return std::pair{buf, "loaded: "s + act.file.name.get()};
        },
        [&] (load_error_action& act) {
            buf.content = act.file.content;
            buf.from = act.file;
            return std::pair{buf, "error while loading: "s + act.file.name.get()};
        },
        [&] (save_progress_action& act) {
            buf.from = act.file;
            return std::pair{buf, ""s};
        },
        [&] (save_done_action& act) {
            buf.from = act.file;
            return std::pair{buf, "saved: "s + act.file.name.get()};
        },
        [&] (save_error_action& act) {
            buf.from = act.file;
            return std::pair{buf, "error while saving: "s + act.file.name.get()};
        })(act);
}

namespace {

std::streamoff stream_size(std::istream& file)
{
    auto begp = file.tellg();
    file.seekg(0, std::ios::end);
    auto endp = file.tellg();
    file.seekg(begp, std::ios::beg);
    return endp - begp;
}

auto load_file_effect(immer::box<std::string> file_name)
{
    constexpr auto progress_report_rate_bytes = 1 << 20;

    return [=] (auto& ctx) {
        ctx.async([=] {
            auto content = text{}.transient();
            auto file = std::ifstream{};
            file.exceptions(std::fstream::badbit | std::fstream::failbit);
            try {
                file.open(file_name);
                file.exceptions(std::fstream::badbit);
                auto file_size = stream_size(file);
                auto progress  = loading_file{ file_name, {}, 0, file_size };
                auto ln1       = std::string{};
                auto ln2       = std::string{};
                auto lastp = progress.loaded_bytes;
                // work-around gcc-7 bug
                // https://www.mail-archive.com/gcc-bugs@gcc.gnu.org/msg533664.html
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
                while (std::getline(file, ln1)) {
#pragma GCC diagnostic pop
                    ln2.clear();
                    utf8::replace_invalid(ln1.begin(), ln1.end(),
                                         std::back_inserter(ln2));
                    content.push_back({begin(ln2), end(ln2)});
                    progress.loaded_bytes += ln1.size();
                    if (progress.loaded_bytes - lastp >
                        progress_report_rate_bytes) {
                        progress.content = content.persistent();
                        ctx.dispatch(load_progress_action{progress});
                        lastp = progress.loaded_bytes;
                    }
                }
                ctx.dispatch(load_done_action{{file_name, content.persistent()}});
            } catch (...) {
                ctx.dispatch(load_error_action{{file_name, content.persistent()},
                                               std::current_exception()});
            }
        });
    };
}

auto save_file_effect(immer::box<std::string> file_name,
                      text old_content,
                      text new_content)
{
    constexpr auto progress_report_rate_lines = (1 << 20) / 40;

    return [=] (auto& ctx) {
        ctx.async([=] {
            auto progress = saving_file{ file_name, new_content, 0 };
            auto file = std::ofstream{};
            file.exceptions(std::fstream::badbit | std::fstream::failbit);
            try {
                file.open(file_name);
                auto lastp = std::size_t{};
                immer::for_each(new_content, [&] (auto l) {
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
                ctx.dispatch(save_done_action{{file_name, new_content}});
            } catch (...) {
                auto content = new_content.take(progress.saved_lines)
                             + old_content.drop(progress.saved_lines);
                ctx.dispatch(save_error_action{{file_name, content},
                                               std::current_exception()});
            }
        });
    };
}

} // anonymous

result<buffer, buffer_action> save_buffer(buffer buf)
{
    auto file = std::get<existing_file>(buf.from);
    buf.from = saving_file{file.name, buf.content, {}};
    return { buf, save_file_effect(file.name, file.content, buf.content) };
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

line get_line(const text& txt, index row)
{
    return row >= 0 && row < (index)txt.size() ? txt[row] : line{};
}

index line_length(const line& ln)
{
    return utf8::unchecked::distance(ln.begin(), ln.end());
}

std::size_t line_char(const line& ln, index col)
{
    auto fst = ln.begin();
    auto lst = ln.end();
    while (col --> 0 && fst != lst)
        utf8::unchecked::next(fst);
    return fst.index();
}

std::pair<std::size_t, std::size_t> line_char_region(const line& ln, index col)
{
    auto fst = ln.begin();
    auto lst = ln.end();
    while (col --> 0 && fst != lst)
        utf8::unchecked::next(fst);
    auto prv = fst;
    if (fst != lst)
        utf8::unchecked::next(fst);
    return { prv.index(), fst.index() };
}

index expand_tabs(const line& ln, index col)
{
    using namespace std;
    auto cur_col = index{};
    for (auto c : line_range(ln)) {
        if (col-- <= 0) break;
        if (c == '\t') {
            cur_col += tab_width - (cur_col % tab_width);
        } else
            ++cur_col;
    }
    return cur_col;
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
        buf.cursor.col = line_length(buf.content[buf.cursor.row]);
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
    auto cur = buf.cursor;
    auto ln  = get_line(buf.content, cur.row);
    auto chr = line_char(ln, cur.col);
    if (chr == 0) {
        if (cur.row > 0) {
            buf.cursor.row -= 1;
            buf.cursor.col = line_length(get_line(buf.content, buf.cursor.row));
        }
    } else  {
        --buf.cursor.col;
        auto new_chr = line_char(ln, buf.cursor.col);
        if (chr == new_chr)
            buf.cursor.col = line_length(ln) - 1;
    }
    return buf;
}

buffer move_cursor_right(buffer buf)
{
    auto cur     = buf.cursor;
    auto ln      = get_line(buf.content, cur.row);
    auto chr     = line_char(ln, cur.col);
    auto new_chr = line_char(ln, cur.col + 1);
    if (chr == new_chr) {
        buf = move_cursor_down(buf);
        buf.cursor.col = 0;
    } else {
        ++buf.cursor.col;
    }
    return buf;
}

buffer scroll_to_cursor(buffer buf, coord wsize)
{
    auto cur = buf.cursor;
    cur.col = expand_tabs(get_line(buf.content, cur.row), cur.col);
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
    auto cur = buf.cursor;
    if (cur.row == (index)buf.content.size()) {
        buf.content = buf.content.push_back({});
        return move_cursor_down(buf);
    } else {
        auto ln = buf.content[cur.row];
        auto chr = line_char(ln, cur.col);
        if (cur.row + 1 < (index)buf.content.size() || chr + 1 < ln.size()) {
            buf.content = buf.content
                .set(cur.row, ln.take(chr))
                .insert(cur.row + 1, ln.drop(chr));
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
    auto cur   = buf.cursor;
    auto ln    = [&] {
        auto ln = line{}.transient();
        utf8::append(value, std::back_inserter(ln));
        return ln.persistent();
    } ();
    if (cur.row == (index)buf.content.size()) {
        buf.content = buf.content.push_back(ln);
    } else {
        buf.content = buf.content.update(cur.row, [&] (auto l) {
            return l.insert(line_char(l, cur.col), ln);
        });
    }
    buf.cursor.col = cur.col + 1;
    return buf;
}

buffer delete_char(buffer buf)
{
    auto cur = buf.cursor;
    buf = move_cursor_left(buf);
    if (cur.col != buf.cursor.col && cur.row == buf.cursor.row) {
        buf.content = buf.content.update(cur.row, [&] (auto l) {
            auto [fst, lst] = line_char_region(l, buf.cursor.col);
            return l.erase(fst, lst);
        });
    } else if (cur.row > 0) {
        auto ln1 = buf.content[cur.row - 1];
        if (cur.row < (index)buf.content.size()) {
            buf.content = buf.content
                .update(cur.row, [&] (auto ln2) { return ln1 + ln2; })
                .erase(cur.row - 1);
        }
    }
    return buf;
}

buffer delete_char_right(buffer buf)
{
    auto cur = buf.cursor;
    buf = move_cursor_right(buf);
    return cur == buf.cursor ? buf : delete_char(buf);
}

std::pair<buffer, text> cut_rest(buffer buf)
{
    if (buf.cursor.row < (index)buf.content.size()) {
        auto ln  = buf.content[buf.cursor.row];
        auto chr = line_char(ln, buf.cursor.col);
        if (chr < ln.size()) {
            buf.content = buf.content.set(buf.cursor.row, ln.take(chr));
            return {buf, {ln.drop(chr)}};
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
    auto cur = buf.cursor;
    if (cur.row < (index)buf.content.size()) {
        auto ln1 = buf.content[cur.row];
        auto chr = line_char(ln1, cur.col);
        buf.content = buf.content.set(cur.row, ln1.take(chr) + paste[0]);
        buf.content = buf.content.take(cur.row + 1)
            + paste.drop(1)
            + buf.content.drop(cur.row + 1);
        auto ln2 = buf.content[cur.row + paste.size() - 1];
        buf.content = buf.content.set(cur.row + paste.size() - 1,
                                      ln2 + ln1.drop(chr));
    } else {
        buf.content = buf.content + paste;
    }
    buf.cursor.row = cur.row + paste.size() - 1;
    buf.cursor.col = paste.size() > 1
        ? line_length(paste.back())
        : cur.col + line_length(paste.back());
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
            .take(ends.row + 1)
            .drop(starts.row)
            .update(ends.row-starts.row, [&] (auto l) {
                return l.take(line_char(l, ends.col));
            })
            .update(0, [&] (auto l) {
                return l.drop(line_char(l, starts.col));
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
                .take(starts.row + 1)
                .update(starts.row, [&] (auto l1) {
                    auto l2 = content[ends.row];
                    return l1.take(line_char(l1, starts.col))
                        +  l2.drop(line_char(l2, ends.col));
                })
              + buf.content.drop(ends.row + 1);
        } else {
            buf.content = buf.content.update(starts.row, [&] (auto l) {
                return l.take(line_char(l, starts.col))
                    +  l.drop(line_char(l, ends.col));
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
    auto cur = buf.cursor;
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
        auto cursor = buf.cursor;
        auto starts = std::min(cursor, *buf.selection_start);
        auto ends   = std::max(cursor, *buf.selection_start);
        starts.col = starts.row < (index)buf.content.size() ? starts.col : 0;
        ends.col   = ends.row < (index)buf.content.size() ? ends.col : 0;
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
