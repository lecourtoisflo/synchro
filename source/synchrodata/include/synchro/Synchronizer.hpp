#pragma once

#include "SynchronizedData.hpp"

#include <vector>

namespace synchro
{
/**
 * @brief Data synchronizer
 *
 * Requirements for R, O and L are the same as for SynchronizedData
 * Requirements for Pooler<T> are:
 * - function boost::signals2::connection onReceived(Callback&& cbk) where Callback is a function type with prototype void(const std::shared_ptr<T>&)
 */
template<template<class> class Pooler, class R, class O = Optional<>, class L = List<>>
class Synchronizer
{
public:
    using Data = SynchronizedData<R, O, L>; ///< Synchronized data type

    /// @brief Trait class to define tuple of Poolers
    template<class T>
    struct Poolers;
    /// @brief Specialization for tuples
    template<class... Ts>
    struct Poolers<std::tuple<Ts...>>
    {
        using type = typename std::tuple<Pooler<Ts>...>; ///< Tuple of poolers type definition
    };
    using RequiredPoolers = typename Poolers<typename R::TupleType>::type; ///< Tuple of poolers for required types definition
    using OptionalPoolers = typename Poolers<typename O::TupleType>::type; ///< Tuple of poolers for optional types definition
    using ListPoolers     = typename Poolers<typename L::TupleType>::type; ///< Tuple of poolers for list types definition

public:
    /**
     * @brief Constructor
     *
     * @param requiredPoolers the tuple of poolers for required types
     * @param optionalPoolers the tuple of poolers for optional types
     * @param listPoolers the tuple of poolers for list types
     */
    Synchronizer(RequiredPoolers&& requiredPoolers, OptionalPoolers&& optionalPoolers = OptionalPoolers{}, ListPoolers&& listPoolers = ListPoolers{})
        : requiredPoolers_(std::forward<RequiredPoolers>(requiredPoolers)),
          optionalPoolers_(std::forward<OptionalPoolers>(optionalPoolers)),
          listPoolers_(std::forward<ListPoolers>(listPoolers))
    {
        connect(requiredPoolers_);
        connect(optionalPoolers_);
        connect(listPoolers_);
    }

    /**
     * @brief Retrieve underlying synchronized data
     * @returns the synchronized data in synchronizer
     */
    Data& data() { return data_; }

    /**
     * @brief Retrieve underlying pooler
     *
     * Function will not compile if T is neither a required, optional and list type of the synchronizer
     *
     * @returns the corresonding underlying pooler for type T
     */
    template<class T>
    Pooler<T>& pooler()
    {
        static_assert(util::Contains<T, typename R::TupleType>() || util::Contains<T, typename O::TupleType>() || util::Contains<T, typename L::TupleType>());
        if constexpr (util::Contains<T, typename R::TupleType>())
        {
            return std::get<Pooler<T>>(requiredPoolers_);
        }
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
    RequiredPoolers requiredPoolers_;
    OptionalPoolers optionalPoolers_;
    ListPoolers listPoolers_;
};
} // namespace synchro
