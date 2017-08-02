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

#include <ewig/application.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

struct _win_st;

namespace ewig {

struct terminal
{
    using action_handler = std::function<void(terminal_action)>;

    terminal(boost::asio::io_service& serv);

    coord size();

    void start(action_handler ev);
    void stop();

private:
    struct cleanup_fn
    {
        void operator() (_win_st* win) const;
    };

    void next_();

    std::unique_ptr<_win_st, cleanup_fn> win_;
    boost::asio::posix::stream_descriptor input_;
    action_handler handler_;
};

} // namespace ewig
