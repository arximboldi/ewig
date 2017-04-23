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

#include <ewig/model.hpp>
#include <ewig/keys.hpp>

namespace ewig {

enum class color
{
    message = 1,
    selection,
};

struct tui
{
    tui();

    key_code read_key();
    coord size();

private:
    struct cleanup_fn
    {
        void operator() (WINDOW* win) const;
    };

    std::unique_ptr<WINDOW, cleanup_fn> win_;
};

int main(int argc, char* argv[]);

} // namespace ewig
