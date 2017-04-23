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

namespace ewig {

using index = int;

struct coord
{
    index row = {};
    index col = {};

    bool operator<(const coord& other) const
    { return row < other.row || (row == other.row && col < other.col); }

    bool operator==(const coord& other) const
    { return row == other.row && col == other.col; }
    bool operator!=(const coord& other) const
    { return !(*this == other); }
};

} // namespace ewig
