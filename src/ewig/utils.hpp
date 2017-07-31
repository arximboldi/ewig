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

#include <immer/vector.hpp>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/functional/hash.hpp>

#include <optional>
#include <tuple>

namespace std
{

template <typename... T>
struct hash<tuple<T...>>
{
    size_t operator()(const tuple<T...>& arg) const noexcept
    {
        return boost::hash_value(arg);
    }
};

template <typename T>
struct hash<immer::vector<T>>
{
    size_t operator()(const immer::vector<T>& arg) const noexcept
    {
        return boost::hash_range(arg.begin(), arg.end()) ;
    }
};

} //namespace std

namespace ewig {

template <typename T, typename Fn>
std::optional<T> optional_map(std::optional<T> v, Fn&& fn)
{
    return v ? std::forward<Fn>(fn)(std::move(*v)) : v;
}

} // namespace ewig
