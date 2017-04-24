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

#include "ewig/keys.hpp"
#include <cassert>

extern "C" {
#include <ncursesw/ncurses.h>
}

using namespace std::string_literals;

namespace ewig {

key_map make_key_map(std::initializer_list<std::pair<key_seq, std::string>> args)
{
    auto map = typename key_map::value_type{};
    for (auto item : args) {
        auto kseq = key_seq{};
        for (auto kcode : item.first) {
            if (!map[kseq].empty())
                throw std::runtime_error{"ambiguous bindings"};
            kseq = kseq.push_back(kcode);
        }
        auto res = map.emplace(std::move(kseq), std::move(item.second));
        if (!res.second)
            throw std::runtime_error{"dupplicate binding"};
    }
    return map;
}

namespace key {

namespace {

key_seq from_special_str(const char* name)
{
    auto id = ::tigetstr(name);
    if (!id || id == (char*)-1)
        throw std::runtime_error{"tigetstr() error for: "s + name};
    auto code = ::key_defined(name);
    if (code <= 0)
        throw std::runtime_error{"key_defined() error for: "s + name};
    return {{KEY_CODE_YES, code}};
}

} // anonymous

key_seq seq(char key)
{
    return {{OK, key}};
}

key_seq seq(special key)
{
    switch(key) {
    case up        : return {{KEY_CODE_YES, KEY_UP}};
    case down      : return {{KEY_CODE_YES, KEY_DOWN}};
    case left      : return {{KEY_CODE_YES, KEY_LEFT}};
    case right     : return {{KEY_CODE_YES, KEY_RIGHT}};
    case home      : return {{KEY_CODE_YES, KEY_HOME}};
    case end       : return {{KEY_CODE_YES, KEY_END}};
    case backspace : return {{KEY_CODE_YES, KEY_BACKSPACE}};
    case delete_   : return {{KEY_CODE_YES, KEY_DC}};
    case page_up   : return {{KEY_CODE_YES, KEY_PPAGE}};
    case page_down : return {{KEY_CODE_YES, KEY_NPAGE}};
    };
    throw std::logic_error("unreachable");
}

key_seq ctrl(char ch)
{
    if (ch >= 'a' && ch <= 'z')
        ch = (ch - 'a') + 'A';
    if (ch < '@' || ch > '_')
        throw std::runtime_error{"bad control key: "s + ch};
    return {{OK, ch - '@'}};
}

key_seq ctrl(special key)
{
    switch (key) {
    case up:    return from_special_str("kUP5");
    case down:  return from_special_str("kDN5");
    case left:  return from_special_str("kLFT5");
    case right: return from_special_str("kRIT5");
    default:
        throw std::runtime_error("unknown control for special key: "s +
                                 std::to_string(key));
    }
}

key_seq alt(char key)
{
    return seq(ctrl('['), key);
}

key_seq alt(special key)
{
    switch (key) {
    case up:    return from_special_str("kUP3");
    case down:  return from_special_str("kDN3");
    case left:  return from_special_str("kLFT3");
    case right: return from_special_str("kRIT3");
    default:
        throw std::runtime_error("unknown control for special key: "s +
                                 std::to_string(key));
    }
}

} // namespace key
} // namespace ewig
