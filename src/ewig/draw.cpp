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

#include "ewig/draw.hpp"

#include <scelta.hpp>

extern "C" {
#include <ncurses.h>
}

namespace ewig{

namespace {

// Fills the string `str` with the display contents of the line `ln`
// between the display columns `first_col` and `first_col + num_col`.
// It takes into account tabs, expanding them correctly, and fills the
// remaining until num_col with spaces.
void display_line_fill(const line& ln, int first_col, int num_col,
                       std::wstring& str)
{
    using namespace std;
    auto cur_col = index{};
    for (auto c : line_range(ln)) {
        if (num_col == cur_col - first_col)
            return;
        else if (c == '\t') {
            auto next_col = cur_col + tab_width - (cur_col % tab_width);
            auto to_fill  = std::min(next_col, first_col + num_col) -
                            std::max(cur_col, first_col);
            std::fill_n(back_inserter(str), to_fill, ' ');
            cur_col = next_col;
        } else if (cur_col >= first_col) {
            str.push_back(c);
            ++cur_col;
        } else
            ++cur_col;
    }
    std::fill_n(back_inserter(str), num_col - (cur_col - first_col), ' ');
}

std::pair<coord, coord> display_selected_region(const buffer& buf)
{
    auto [starts, ends] = selected_region(buf);
    starts.col  = expand_tabs(get_line(buf.content, starts.row), starts.col);
    ends.col    = expand_tabs(get_line(buf.content, ends.row), ends.col);
    starts.row -= buf.scroll.row;
    ends.row   -= buf.scroll.row;
    starts.col -= buf.scroll.col;
    ends.col   -= buf.scroll.col;
    return {starts, ends};
}

} // anonymous namespace

void draw_text(const buffer& buf, coord size)
{
    using namespace std;
    int col, row;
    attrset(A_NORMAL);
    getyx(stdscr, col, row);

    auto str      = std::wstring{};
    auto first_ln = begin(buf.content) + min(buf.scroll.row,
                                             (index)buf.content.size());
    auto last_ln  = begin(buf.content) + min(size.row + buf.scroll.row,
                                             (index)buf.content.size());
    auto [starts, ends] = display_selected_region(buf);

    immer::for_each(first_ln, last_ln, [&] (auto ln) {
        str.clear();
        display_line_fill(ln, buf.scroll.col + col, 2*size.col, str);
        ::move(row, col);
        auto in_selection = row >= starts.row && row <= ends.row;
        if (in_selection) {
            auto hl_first = row == starts.row ? std::max(starts.col, 0) : 0;
            auto hl_last  = row == ends.row   ? std::max(ends.col, 0) : str.size();
            ::addnwstr(str.c_str(), hl_first);
            ::attron(COLOR_PAIR(color::selection));
            ::addnwstr(str.c_str() + hl_first, hl_last - hl_first);
            ::attroff(COLOR_PAIR(color::selection));
            ::addnwstr(str.c_str() + hl_last, str.size() - hl_last);
        } else {
            ::addwstr(str.c_str());
        }
        row++;
    });
}

void draw_mode_line(const buffer& buf, index maxcol)
{
    attrset(A_REVERSE);
    auto dirty_mark = is_dirty(buf) ? "**" : "--";
    auto file_name = scelta::match([](auto&& f) { return f.name; })(buf.from);
    auto cur = buf.cursor;
    cur.col = expand_tabs(get_line(buf.content, cur.row), cur.col);
    ::printw(" %s %s  (%d, %d)",
            dirty_mark,
            file_name.get().c_str(),
            cur.col, cur.row);
    ::hline(' ', maxcol);

    scelta::match(
        [&] (const saving_file& file) {
            auto str        = std::string{"saving..."};
            auto size       = std::max(file.content.size(), std::size_t{1});
            auto progress   = (float)file.saved_lines / size;
            auto percentage = int(progress * 100);
            ::move(getcury(stdscr), maxcol - str.size() - 6);
            attrset(A_NORMAL | A_BOLD);
            ::attron(COLOR_PAIR(color::mode_line_message));
            ::printw(" %s %*d%% ", str.c_str(), 2, percentage);
        },
        [&] (const loading_file& file) {
            auto str        = std::string{"loading..."};
            auto progress   = (float)file.loaded_bytes / file.total_bytes;
            auto percentage = int(progress * 100);
            ::move(getcury(stdscr), maxcol - str.size() - 6);
            attrset(A_NORMAL | A_BOLD);
            ::attron(COLOR_PAIR(color::mode_line_message));
            ::printw(" %s %*d%% ", str.c_str(), 2, percentage);
        },
        [](auto&&) {})(buf.from);
}

void draw_message(const message& msg)
{
    attrset(A_NORMAL);
    ::attron(COLOR_PAIR(color::message));
    ::addstr(" ");
    ::addstr(msg.content.get().c_str());
    ::attroff(COLOR_PAIR(color::message));
}

void draw_text_cursor(const buffer& buf, coord window_size)
{
    auto cur = buf.cursor;
    cur.col = expand_tabs(get_line(buf.content, cur.row), cur.col);
    ::move(cur.row - buf.scroll.row, cur.col - buf.scroll.col);
    ::curs_set(cur.col >= buf.scroll.col &&
               cur.row >= buf.scroll.row &&
               cur.col < buf.scroll.col + window_size.col &&
               cur.row < buf.scroll.row + window_size.row);
}

void draw(const application& app)
{
    ::erase();

    auto size = editor_size(app);
    ::move(0, 0);
    draw_text(app.current, size);

    ::move(size.row, 0);
    draw_mode_line(app.current, size.col);

    if (!app.messages.empty()) {
        ::move(size.row + 1, 0);
        draw_message(app.messages.back());
    }

    draw_text_cursor(app.current, size);
    ::refresh();
}

} // namespace ewig
