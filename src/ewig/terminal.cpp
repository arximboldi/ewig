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

#include "ewig/terminal.hpp"
#include "ewig/draw.hpp"

#include <boost/asio/read.hpp>

#include <iostream>

extern "C" {
#include <ncurses.h>
}

using namespace std::placeholders;
using namespace std::string_literals;

namespace ewig {

terminal::terminal(boost::asio::io_service& serv)
    : win_{::initscr()}
    , input_{serv, ::dup(STDIN_FILENO)}
    , signal_{serv, SIGWINCH}
{
    if (win_.get() != ::stdscr)
        throw std::runtime_error{"error while initializing ncurses"};

    ::raw();
    ::noecho();
    ::keypad(stdscr, true);
    ::nodelay(stdscr, true);

    ::start_color();
    ::use_default_colors();
    ::init_pair((int)color::message,   COLOR_YELLOW, -1);
    ::init_pair((int)color::selection, COLOR_BLACK, COLOR_YELLOW);
    ::init_pair((int)color::mode_line_message, COLOR_WHITE, COLOR_RED);
}

coord terminal::size()
{
    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);
    return {maxrow, maxcol};
}

void terminal::start(action_handler ev)
{
    assert(!handler_);
    handler_ = std::move(ev);
    next_key_();
    next_resize_();
}

void terminal::stop()
{
    input_.cancel();
    signal_.cancel();
    handler_ = {};
}

void terminal::next_resize_()
{
    signal_.async_wait([=] (auto ec, auto) {
        if (!ec) {
            next_resize_();
            auto ws = ::winsize{};
            if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
                ::perror("TIOCGWINSZ");
            else {
                ::resizeterm(ws.ws_row, ws.ws_col);
                handler_(resize_action{{ws.ws_row, ws.ws_col}});
            }
        }
    });
}

void terminal::next_key_()
{
    using namespace boost::asio;
    input_.async_read_some(null_buffers(), [&] (auto ec, auto) {
        if (!ec) {
            auto key = wint_t{};
            auto res = int{};
            while (ERR != (res = ::wget_wch(win_.get(), &key))) {
                next_key_();
                handler_(key_action{{res, key}});
            }
        }
    });
}

void terminal::cleanup_fn::operator() (WINDOW* win) const
{
    if (win) {
        // consume all remaining characters from the terminal so they
        // don't leak in the bash prompt after quitting, then restore
        // the terminal state
        auto key = wint_t{};
        while (::get_wch(&key) != ERR);
        ::endwin();
    }
}

} // namespace ewig
