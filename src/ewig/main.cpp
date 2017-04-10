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

#include "ewig/main.hpp"

#include <immer/flex_vector_transient.hpp>
#include <immer/algorithm.hpp>
#include <boost/optional.hpp>

#include <ncurses.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

namespace ewig {

namespace {

file_buffer load_file(const char* file_name)
{
    auto file = std::ifstream{file_name};
    auto content = text{}.transient();

    auto ln = std::string{};
    while (std::getline(file, ln))
        content.push_back({begin(ln), end(ln)});

    return { content.persistent(), {}, {}, file_name, false };
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
    auto cur = actual_cursor(buf);
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

boost::optional<app_state> handle_key(app_state state, int key)
{
    using namespace std::string_literals;

    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);
    // make space for minibuffer and modeline
    auto window_size = coord{maxrow - 2, maxcol};

    if (keybound(key, 0)) {
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
        default:
            break;
        }
        return put_message(state, "ncurses key pressed: "s + keyname(key));
    } else if (std::iscntrl(key)) {
        switch (key) {
        case '\003': // ctrl-c
            return {};
        case '\033': // escape
            return {};
        default:
            break;
        }
        return put_message(state, "control key pressed: "s + keyname(key));
    } else if (key == KEY_RESIZE) {
        return put_message(state, "terminal resized");
    } else {
        return put_message(state, "adding character: "s + keyname(key));
    }
    return state;
}

void draw_text(const text& t, coord scroll, coord size)
{
    using namespace std;
    attrset(A_NORMAL);
    int col, row;
    getyx(stdscr, col, row);
    auto str      = std::string{};
    auto first_ln = begin(t) + min(scroll.row, (index)t.size());
    auto last_ln  = begin(t) + min(size.row + scroll.row, (index)t.size());
    immer::for_each(first_ln, last_ln, [&] (auto l) {
        str.clear();
        auto first_ch = begin(l) + min(scroll.col, (index)l.size());
        auto last_ch  = begin(l) + min(size.col + scroll.col, (index)l.size());
        immer::copy(first_ch, last_ch, back_inserter(str));
        move(row++, col);
        printw("%s", str.c_str());
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
    printw("message: %s", msg.content.get().c_str());
}

void draw_text_cursor(coord cursor, coord scroll, coord size)
{
    move(cursor.row - scroll.row, cursor.col - scroll.col);
    curs_set(cursor.col >= scroll.col &&
             cursor.row >= scroll.row &&
             cursor.col < scroll.col + size.col &&
             cursor.row < scroll.row + size.row);
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

    draw_text_cursor(actual_cursor(app.buffer), app.buffer.scroll, size);
    refresh();
}

struct tui
{
    app_state state;

    tui(const char* file_name)
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

    ~tui()
    {
        endwin();
    }

    int run()
    {
        while (auto new_state = handle_key(state, getch())) {
            state = *new_state;
            draw(state);
        }
        return 0;
    }
};

} // anonymous namespace

int run(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Give me a file name." << std::endl;
        return 1;
    }

    tui editor{argv[1]};
    editor.run();

    return 0;
}

} // namespace ewig
