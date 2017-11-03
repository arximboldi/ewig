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

#include <lager/store.hpp>
#include <lager/event_loop/boost_asio.hpp>

#include <lager/debug/enable.hpp>
#include <lager/debug/http_server.hpp>
#include <lager/debug/cereal/immer_vector.hpp>
#include <lager/debug/cereal/immer_box.hpp>
#include <lager/debug/cereal/tuple.hpp>
#include <cereal/types/unordered_map.hpp>

#include <iostream>

namespace cereal {

LAGER_CEREAL_STRUCT(std::monostate);

// custom serialization of text to make text look prettier by looking
// like a list of strings, as opposed to just a list of numbers

template <typename Archive>
void save(Archive& ar, const ewig::text& txt)
{
    ar(make_size_tag(txt.size()));
    immer::for_each(txt, [&] (auto&& ln) {
        auto lns = std::string{ln.begin(), ln.end()};
        ar(lns);
    });
};

template <typename Archive>
void load(Archive& ar, ewig::text& txt)
{
    auto sz = std::size_t{};
    ar(make_size_tag(sz));
    auto t = std::move(txt).transient();
    for (auto i = std::size_t{}; i < sz; ++i) {
        std::string x;
        ar(x);
        t.push_back({x.begin(), x.end()});
    }
    txt = std::move(t).persistent();
};

} // namespace cereal

namespace ewig {
namespace {

const auto key_map_emacs = make_key_map(
{
    {key::seq(key::ctrl('p')), "move-up"},
    {key::seq(key::up),        "move-up"},
    {key::seq(key::down),      "move-down"},
    {key::seq(key::ctrl('n')), "move-down"},
    {key::seq(key::ctrl('b')), "move-left"},
    {key::seq(key::left),      "move-left"},
    {key::seq(key::ctrl('f')), "move-right"},
    {key::seq(key::right),     "move-right"},
    {key::seq(key::page_down), "page-down"},
    {key::seq(key::page_up),   "page-up"},
    {key::seq(key::backspace), "delete-char"},
    {key::seq(key::backspace_),"delete-char"},
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

void run(int argc, const char** argv, const std::string& fname)
{
    auto debugger = lager::http_debug_server{argc, argv, 8080};
    auto serv = boost::asio::io_service{};
    auto term = terminal{serv};
    auto st   = lager::make_store<action>(
        application{term.size(), key_map_emacs},
        update,
        draw,
        lager::boost_asio_event_loop{serv, [&] { term.stop(); }},
        lager::enable_debug(debugger));
    term.start([&] (auto ev) { st.dispatch (ev); });
    st.dispatch(command_action{"load", fname});
    serv.run();
}

} // anonymous
} // namespace ewig

int main(int argc, const char* argv[])
{
    std::locale::global(std::locale(""));
    ::setlocale(LC_ALL, "");

    if (argc != 2) {
        std::cerr << "give me a file name" << std::endl;
        return 1;
    }

    ewig::run(argc, argv, argv[1]);
    return 0;
}
