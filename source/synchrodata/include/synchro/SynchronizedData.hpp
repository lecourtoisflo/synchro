#pragma once

#include "Broadcaster.hpp"
#include <list>
#include <memory>
#include <tuple>
#include <type_traits>

namespace synchro
{

template<class T>
struct ValueType
{
    using type = T;
};

template<template<class> class V, class T>
struct ValueType<V<T>>
{
    using type = T;
};
template<class T>
using ValueTypeTuple = ValueType<std::tuple<T>>;

template<class... Ts>
struct Required
{
    using TupleType = std::tuple<Ts...>;
};

template<class... Ts>
struct Optional
{
    using TupleType = std::tuple<Ts...>;
};

template<class... Ts>
struct List
{
    using TupleType = std::tuple<Ts...>;
};

struct Utils
{
    template<class U, class... Ts>
    static constexpr bool contains()
    {
        return std::disjunction_v<std::is_same<U, Ts>...>;
    }
};

template<class R, class O, class L>
class SynchronizedData
{
public:
    template<class T>
    typename Broadcaster<T>::Connection onReceived(typename Broadcaster<T>::Callback&& cbk)
    {
        if constexpr (Utils::contains<T, Type<R>>())
        {
            return std::get<Broadcaster<T>>(requiredBroadcasters_)
                .onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
        else if constexpr (Utils::contains<T, Type<O>>())
        {
            return std::get<Broadcaster<T>>(optionalBroadcasters_)
                .onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
        else if constexpr (Utils::contains<T, Type<L>>())
        {
            return std::get<Broadcaster<T>>(listBroadcasters_)
                .onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
    }

    template<class T>
    void send(const std::shared_ptr<T>& data)
    {
        if constexpr (Utils::contains<T, Type<R>>())
        {
            std::get<std::shared_ptr<T>>(requiredData_) = data;
            if (areAllRequiredPresent())
            {
                std::get<Broadcaster<T>>(requiredBroadcasters_).send(data);
                std::get<std::shared_ptr<T>>(requiredData_).reset();

                // TODO send signals waiting
                // Clear
                clearAlldata();
            }
        }
        else if constexpr (Utils::contains<T, Type<O>>())
        {
            if (areAllRequiredPresent())
            {
                std::get<Broadcaster<T>>(optionalBroadcasters_).send(data);
                return;
            }
            std::get<std::shared_ptr<T>>(optionalData_) = data;
        }
        else if constexpr (Utils::contains<T, Type<L>>())
        {
            if (areAllRequiredPresent())
            {
                std::get<Broadcaster<T>>(requiredBroadcasters_).send(data);
                return;
            }
            std::get<std::list<std::shared_ptr<T>>>(listData_).push_back(data);
        }
    }

private:
    template<class T>
    using Type = typename ValueTypeTuple<typename T::TupleType>::type;

private:
    bool areAllRequiredPresent() const
    {
        return std::apply([](auto&... ptr) { return (... && (ptr.use_count() > 0)); },
                          requiredData_);
    }

    void clearAlldata()
    {
        auto clear = [](auto&... ptr) { (..., ptr.reset()); };
        std::apply(clear, requiredData_);
        std::apply(clear, optionalData_);
        std::apply([](auto&... list) { (..., list.clear()); }, listData_);
    }

private:
    std::tuple<Broadcaster<Type<R>>> requiredBroadcasters_;
    std::tuple<Broadcaster<Type<O>>> optionalBroadcasters_;
    std::tuple<Broadcaster<Type<L>>> listBroadcasters_;

    std::tuple<std::shared_ptr<Type<R>>> requiredData_;
    std::tuple<std::shared_ptr<Type<O>>> optionalData_;
    std::tuple<std::list<std::shared_ptr<Type<L>>>> listData_;
};

} // namespace synchro
