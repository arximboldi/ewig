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

#include <boost/asio/io_service.hpp>
#include <future>
#include <type_traits>

struct io_service : boost::asio::io_service
{
    using base_t = boost::asio::io_service;
    using base_t::base_t;

    template <typename Fn>
    void async(Fn&& fn)
    {
        static_assert(std::is_same_v<decltype(fn()), void>, "");
        collect_pending_();
        pending_.push_back(std::async(std::forward<Fn>(fn)));
    }

    auto run()
    {
        auto result = base_t::run();
        join_pending_();
        return result;
    }

private:
    void join_pending_()
    {
        for (auto&& f : pending_)
            f.wait();
    }

    void collect_pending_()
    {
        std::remove_if(pending_.begin(), pending_.end(), [] (auto&& f) {
            return f.wait_for(std::chrono::seconds{0}) == std::future_status::ready;
        });
    }

    std::vector<std::future<void>> pending_;
};
