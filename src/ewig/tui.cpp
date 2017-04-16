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

#include "ewig/tui.hpp"

#include <immer/flex_vector_transient.hpp>
#include <immer/algorithm.hpp>
#include <boost/optional.hpp>

#include <ncursesw/ncurses.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

namespace ewig {

namespace {

void display_line_fill(const line& ln, int first_col, int num_col,
                       std::wstring& str)
{
    using namespace std;
    auto cur_col = index{};
    for (auto c : ln) {
        if (num_col == (index)str.size())
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
}

void draw_text(const text& t, coord scroll, coord size)
{
    using namespace std;
    attrset(A_NORMAL);
    int col, row;
    getyx(stdscr, col, row);
    auto str      = std::wstring{};
    auto first_ln = begin(t) + min(scroll.row, (index)t.size());
    auto last_ln  = begin(t) + min(size.row + scroll.row, (index)t.size());
    immer::for_each(first_ln, last_ln, [&] (auto ln) {
        str.clear();
        display_line_fill(ln, scroll.col + col, size.col, str);
        move(row++, col);
        addwstr(str.c_str());
    });
}

void draw_mode_line(const file_buffer& buffer, int maxcol)
{
    attrset(A_REVERSE);
    printw(" %s  (%d, %d)",
           buffer.file_name.get().c_str(),
           buffer.cursor.col,
           buffer.cursor.row);
    hline(' ', maxcol);
}

void draw_message(const message& msg)
{
    attrset(A_NORMAL);
    addstr("message: ");
    addstr(msg.content.get().c_str());
}

void draw_text_cursor(const file_buffer& buf, coord window_size)
{
    auto cursor = actual_display_cursor(buf);
    move(cursor.row - buf.scroll.row, cursor.col - buf.scroll.col);
    curs_set(cursor.col >= buf.scroll.col &&
             cursor.row >= buf.scroll.row &&
             cursor.col < buf.scroll.col + window_size.col &&
             cursor.row < buf.scroll.row + window_size.row);
}

void draw(const app_state& app)
{
    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);

    erase();

    move(0, 0);
    auto size = coord{(index)std::max(0, maxrow - 2), (index)maxcol};
    draw_text(app.buffer.content, app.buffer.scroll, size);

    move(maxrow - 2, 0);
    draw_mode_line(app.buffer, maxcol);

    if (!app.messages.empty()) {
        move(maxrow - 1, 0);
        draw_message(app.messages.back());
    }

    draw_text_cursor(app.buffer, size);
    refresh();
}

boost::optional<app_state> handle_key(app_state state, int res, wint_t key)
{
    using namespace std::string_literals;

    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);
    // make space for minibuffer and modeline
    auto window_size = coord{maxrow - 2, maxcol};

    if (res == ERR) {
        return put_message(state, "error reading character");
    } else if (res == KEY_CODE_YES) {
        switch (key) {
        case KEY_UP:
            state.buffer = scroll_to_cursor(move_cursor_up(state.buffer),
                                            window_size);
            break;
        case KEY_DOWN:
            state.buffer = scroll_to_cursor(move_cursor_down(state.buffer),
                                            window_size);
            break;
        case KEY_LEFT:
            state.buffer = scroll_to_cursor(move_cursor_left(state.buffer),
                                            window_size);
            break;
        case KEY_RIGHT:
            state.buffer = scroll_to_cursor(move_cursor_right(state.buffer),
                                            window_size);
            break;
        case KEY_BACKSPACE:
            state.buffer = scroll_to_cursor(delete_char(state.buffer),
                                            window_size);
            break;
        case KEY_DC:
            state.buffer = scroll_to_cursor(delete_char_right(state.buffer),
                                            window_size);
            break;
        case KEY_PPAGE:
            state.buffer = page_up(state.buffer, window_size);
            break;
        case KEY_NPAGE:
            state.buffer = page_down(state.buffer, window_size);
            break;
        default:
            break;
        }
        return put_message(state, "ncurses key pressed: "s + keyname(key));
    } else if (std::iscntrl(key)) {
        switch (key) {
        case L'\001': // ctrl-a
            state.buffer = scroll_to_cursor(move_line_start(state.buffer),
                                            window_size);
            break;
        case L'\003': // ctrl-c
            return {};
        case L'\005': // ctrl-e
            state.buffer = scroll_to_cursor(move_line_end(state.buffer),
                                            window_size);
            break;
        case L'\011': // ctrl-i/tab
            state.buffer = scroll_to_cursor(insert_char(state.buffer, '\t'),
                                            window_size);
            break;
        case L'\012': // ctrl-j/intro
            state.buffer = scroll_to_cursor(insert_new_line(state.buffer),
                                            window_size);
            break;
        case L'\013': // ctrl-k
            state.buffer = scroll_to_cursor(delete_rest(state.buffer),
                                            window_size);
            break;
        case L'\033': // escape
            return {};
        default:
            break;
        }
        return put_message(state, "control key pressed: "s + key_name(key));
    } else if (key == KEY_RESIZE) {
        return put_message(state, "terminal resized");
    } else {
        state.buffer = scroll_to_cursor(insert_char(state.buffer, key),
                                        window_size);
        return put_message(state, "adding character: "s + key_name(key));
    }
    return state;
}

} // anonymous namespace

tui::tui(const char* file_name)
    : state{load_file(file_name)}
{
    initscr();
    raw();
    noecho();
    keypad(stdscr, true);
    start_color();
    use_default_colors();
    draw(state);
}

tui::~tui()
{
    endwin();
}

int tui::run()
{
    auto key = wint_t{};
    auto res = get_wch(&key);
    while (auto new_state = handle_key(state, res, key)) {
        state = *new_state;
        draw(state);
        res = get_wch(&key);
        }
    return 0;
}

} // namespace ewig
