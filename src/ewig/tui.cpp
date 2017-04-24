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
#include "ewig/draw.hpp"

#include <iostream>

extern "C" {
#include <ncursesw/ncurses.h>
}

using namespace std::placeholders;
using namespace std::string_literals;

namespace ewig {

namespace {

const auto key_map_emacs = make_key_map(
{
    {key::seq(key::up),        "move-up"},
    {key::seq(key::down),      "move-down"},
    {key::seq(key::left),      "move-left"},
    {key::seq(key::right),     "move-right"},
    {key::seq(key::page_down), "page-down"},
    {key::seq(key::page_up),   "page-up"},
    {key::seq(key::backspace), "delete-char"},
    {key::seq(key::delete_),   "delete-char-right"},
    {key::seq(key::home),      "move-beginning-of-line"},
    {key::seq(key::ctrl('a')), "move-beginning-of-line"},
    {key::seq(key::end),       "move-end-of-line"},
    {key::seq(key::ctrl('e')), "move-end-of-line"},
    {key::seq(key::ctrl('i')), "insert-tab"}, // tab
    {key::seq(key::ctrl('j')), "new-line"}, // enter
    {key::seq(key::ctrl('k')), "kill-line"},
    {key::seq(key::ctrl('w')), "cut"},
    {key::seq(key::ctrl('y')), "paste"},
    {key::seq(key::ctrl('@')), "start-selection"}, // ctrl-space
    {key::seq(key::ctrl('_')), "undo"},
    {key::seq(key::ctrl('x'), key::ctrl('C')), "quit"},
    {key::seq(key::ctrl('x'), key::ctrl('S')), "save"},
    {key::seq(key::ctrl('x'), 'h'), "select-whole-buffer"},
    {key::seq(key::ctrl('x'), '['), "move-beginning-buffer"},
    {key::seq(key::ctrl('x'), ']'), "move-end-buffer"},
    {key::seq(key::alt('w')),  "copy"},
});

} // anonymous

tui::tui()
    : win_{initscr()}
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

key_code tui::read_key()
{
    auto key = wint_t{};
    auto res = ::wget_wch(win_.get(), &key);
    return {res, key};
}

coord tui::size()
{
    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);
    return {maxrow, maxcol};
}

void tui::cleanup_fn::operator() (WINDOW* win) const
{
    if (win) ::endwin();
}

int main(int argc, char* argv[])
{
    std::locale::global(std::locale(""));
    if (argc != 2) {
        std::cerr << "give me a file name" << std::endl;
        return 1;
    }

    auto state = application{load_buffer(argv[1]), key_map_emacs};
    auto ui = tui{};
    draw(state, ui.size());
    while (auto new_state = handle_key(state, ui.read_key(), ui.size())) {
        state = *new_state;
        draw(state, ui.size());
    }

    return 0;
}

} // namespace ewig
