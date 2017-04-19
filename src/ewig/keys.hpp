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

#pragma once

#include <ewig/utils.hpp>

#include <ncursesw/ncurses.h>

#include <tuple>
#include <unordered_map>
#include <vector>

namespace ewig {

using key_code = std::tuple<int, wint_t>;
using key_seq  = std::vector<key_code>;
using key_map  = std::unordered_map<key_seq, std::string>;

// Builds a keymap from `args`.  It also associates all key sequence
// prefixes to the empty string, and checks for ambiguous key
// command sequences.
key_map make_key_map(std::initializer_list<std::pair<key_seq, std::string>> args);

namespace key {

enum special
{
    up        = KEY_UP,
    down      = KEY_DOWN,
    left      = KEY_LEFT,
    right     = KEY_RIGHT,
    home      = KEY_HOME,
    end       = KEY_END,
    backspace = KEY_BACKSPACE,
    delete_   = KEY_DC,
    page_up   = KEY_PPAGE,
    page_down = KEY_NPAGE,
};

key_seq ctrl(char key);
key_seq ctrl(special key);
key_seq alt(char key);
key_seq alt(special key);

key_seq seq(char key);
key_seq seq(special key);

inline key_seq seq(key_seq xs)
{
    return xs;
}

template <typename T, typename ...Ts>
auto seq(T arg, Ts... args)
{
    auto x  = seq(arg);
    auto xs = seq(args...);
    x.insert(x.end(), xs.begin(), xs.end());
    return x;
}

} // namespace key
} // namespace ewig
