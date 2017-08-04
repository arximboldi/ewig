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

#include <functional>
#include <future>

namespace ewig {

/**
 * Context in which *effects* that can generate *actions* of type
 * `Action` can be executed.  Values of type `context` have reference
 * semantics.
 */
template <typename Action>
struct context
{
    using action_t      = Action;
    using service_t     = boost::asio::io_service;
    using service_ref_t = std::reference_wrapper<service_t>;
    using finish_t      = std::function<void()>;
    using dispatch_t    = std::function<void(action_t)>;
    using effect_t      = std::function<void(const context&)>;

    std::reference_wrapper<service_t> service;
    finish_t finish;
    dispatch_t dispatch;

    context(const context& ctx) = default;
    context(context&& ctx) = default;

    template <typename Action_>
    context(const context<Action_>& ctx)
        : service(ctx.service)
        , finish(ctx.finish)
        , dispatch(ctx.dispatch)
    {}

    context(service_t& serv, finish_t fn, dispatch_t ds)
        : service(serv)
        , finish(std::move(fn))
        , dispatch(std::move(ds))
    {}

    template <typename Effect>
    void post(Effect&& eff)
    {
        service.get().post(
            [this, eff = std::forward<Effect>(eff)] {
                effect(*this);
            });
    }
};

/**
 * An effect is a function with side-effects that is executed in a
 * context.  The effect is passed in the context in which it is
 * executed, so it can use it to dispatch new actions or post effects.
 */
template <typename Action>
using effect = typename context<Action>::effect_t;

/**
 * A noop function that can be used as a noop effect.
 */
constexpr auto noop = [] (auto&&...) {};

/**
 * An immutable value that represents an effect that is bound to it.
 * It can be used in pure code to generate effects that depend on the
 * completion of previous effects, cancel on-going asynchronous
 * effects, etc.
 */
struct effect_token
{
    bool operator==(const effect_token& other)
    { return data_ == other.data_; }

    bool operator!=(const effect_token& other)
    { return data_ != other.data_; }

    auto cancel() const
    {
        return [d = data_] (auto&&...) {
            d->cancelled = true;
        };
    }

    template <typename Effect>
    auto cancel_then(Effect&& effect) const
    {
        return [data   = data_,
                effect = std::forward<Effect>(effect)]
            (auto& ctx) {
            assert(!data->cancelled &&
                   "effect already cancelled");
            data->next = std::move(effect);
            data->cancelled = true;
        };
    }

    template <typename Context>
    struct handle
    {
        handle(handle&&) = default;
        handle& operator=(handle&&) = default;

        handle(const handle&) = delete;
        handle& operator=(const handle&) = delete;

        handle(Context ctx, effect_token hdl)
            : ctx_{std::move(ctx)}
            , token_{std::move(hdl)}
        {
            assert(1 == ++token_->data_->debug_count,
                   "only one effect can use the token!");
        }

        ~handle()
        {
            if (hdl_->data_) {
                if
            }
        }

        operator bool() const
        { return !token_.data_->cancelled; }

    private:
        context      ctx_;
        effect_token token_;
    };

    friend effect_with_token()

private:
    friend handle;

    struct data {
        std::atomic<bool> cancelled{false}
        std::atomic<bool> done{false};
        std::atomic<int>  debug_count{0};
        effect<Action>    next{noop};
    };
    std::shared_ptr<data> data_ = std::make_shared<data>();
};

template <typename Effect>
auto async_effect(Effect&& fn) const
{
    auto token = effect_token{};
    return std::make_pair(
        std::thread([fn=std::move(fn),
                    work=boost::asio::io_service::work(service)] {
            fn();
        }).detach();
        )
}

} // namespace ewig
