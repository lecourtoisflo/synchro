#pragma once

#include "SynchronizedData.hpp"

#include <vector>

/**
 * @brief Data synchronizer
 *
 * Requirements for R, O and L are the same as for SynchronizedData
 * Requirements for Pooler are:
 * - default constructible
 * - function boost::signals2::connection onReceived(Callback&& cbk) where Callback is function void(const std::shared_ptr<T>&)
 */
namespace synchro
{
template<template<class> class Pooler, class R, class O = Optional<>, class L = List<>>
class Synchronizer
{
public:
    using Data = SynchronizedData<R, O, L>;

public:
    Synchronizer()
    {
        connect(requiredPoolers_);
        connect(optionalPoolers_);
        connect(listPoolers_);
    }

    Data& data() { return data_; }

    template<class T>
    Pooler<T>& pooler()
    {
        static_assert(util::Contains<T, typename R::TupleType>() || util::Contains<T, typename O::TupleType>() || util::Contains<T, typename L::TupleType>());
        if constexpr (util::Contains<T, typename R::TupleType>()) { return std::get<Pooler<T>>(requiredPoolers_); }
        else if constexpr (util::Contains<T, typename O::TupleType>())
        {
            return std::get<Pooler<T>>(optionalPoolers_);
        }
        else if constexpr (util::Contains<T, typename L::TupleType>())
        {
            return std::get<Pooler<T>>(listPoolers_);
        }
    }

private:
    template<class T>
    struct Poolers;
    template<class... Ts>
    struct Poolers<std::tuple<Ts...>>
    {
        using type = typename std::tuple<Pooler<Ts>...>;
    };
    using Connection = boost::signals2::connection;

private:
    template<class T>
    void connect(Pooler<T>& pooler)
    {
        // redirect pooler on synchronized data
        auto send       = [this](const std::shared_ptr<T>& data) { data_.send(data); };
        auto connection = pooler.onReceived(send);
        connections_.push_back(connection);
    }

    template<class... Ts>
    void connect(std::tuple<Pooler<Ts>...>& poolers)
    {
        std::apply([this](auto&... pooler) { (..., connect(pooler)); }, poolers);
    }

private:
    std::vector<Connection> connections_;
    Data data_;
    typename Poolers<typename R::TupleType>::type requiredPoolers_;
    typename Poolers<typename O::TupleType>::type optionalPoolers_;
    typename Poolers<typename L::TupleType>::type listPoolers_;
};
} // namespace synchro
