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
#include <ncursesw/ncurses.h>
}

using namespace std::placeholders;
using namespace std::string_literals;

namespace ewig {

terminal::terminal(boost::asio::io_service& serv)
    : win_{initscr()}
    , input_{serv, ::dup(STDIN_FILENO)}
{
    if (win_.get() != ::stdscr)
        throw std::runtime_error{"error while initializing ncurses"};

    ::raw();
    ::noecho();
    ::keypad(stdscr, true);

    ::start_color();
    ::use_default_colors();
    ::init_pair((int)color::message,   COLOR_YELLOW, -1);
    ::init_pair((int)color::selection, COLOR_BLACK, COLOR_YELLOW);
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
    next_();
}

void terminal::stop()
{
    input_.cancel();
    handler_ = {};
}

void terminal::next_()
{
    using namespace boost::asio;
    input_.async_read_some(null_buffers(), [&] (auto ec, auto) {
        if (!ec) {
            auto key = wint_t{};
            auto res = ::wget_wch(win_.get(), &key);
            next_();
            handler_({{res, key}, size()});
        }
    });
}

void terminal::cleanup_fn::operator() (WINDOW* win) const
{
    if (win) ::endwin();
}

} // namespace ewig
