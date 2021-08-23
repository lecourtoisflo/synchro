#pragma once

#include "Broadcaster.hpp"
#include <list>
#include <memory>
#include <tuple>
#include <type_traits>

namespace synchro
{

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

template<class T, class Tuple>
struct Contains;
template<class T, class... Ts>
struct Contains<T, std::tuple<Ts...>> : std::disjunction<std::is_same<T, Ts>...>
{
};

template<class R, class O, class L>
class SynchronizedData
{
public:
    template<class T>
    using Connection = typename Broadcaster<T>::Connection;

public:
    template<class T>
    Connection<T> onReceived(typename Broadcaster<T>::Callback&& cbk)
    {
        if constexpr (Contains<T, typename R::TupleType>())
        {
            return std::get<Broadcaster<T>>(requiredBroadcasters_)
                .onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
        // else if constexpr (Contains<T, typename O::TupleType>())
        // {
        //     return std::get<Broadcaster<T>>(optionalBroadcasters_)
        //         .onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        // }
        // else if constexpr (Contains<T, typename L::TupleType>())
        // {
        //     return std::get<Broadcaster<T>>(listBroadcasters_)
        //         .onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        // }
        return Connection<T>();
    }

    template<class T>
    void send(const std::shared_ptr<T>& data)
    {
        if constexpr (Contains<T, typename R::TupleType>())
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
        // else if constexpr (Utils::contains<T, Type<O>>())
        // {
        //     if (areAllRequiredPresent())
        //     {
        //         std::get<Broadcaster<T>>(optionalBroadcasters_).send(data);
        //         return;
        //     }
        //     std::get<std::shared_ptr<T>>(optionalData_) = data;
        // }
        // else if constexpr (Utils::contains<T, Type<L>>())
        // {
        //     if (areAllRequiredPresent())
        //     {
        //         std::get<Broadcaster<T>>(requiredBroadcasters_).send(data);
        //         return;
        //     }
        //     std::get<std::list<std::shared_ptr<T>>>(listData_).push_back(data);
        // }
    }

private:
    template<class T>
    struct Broadcasters;
    template<class... Ts>
    struct Broadcasters<std::tuple<Ts...>> : public std::tuple<Broadcaster<Ts>...>
    {
    };
    template<class T>
    struct Data;
    template<class... Ts>
    struct Data<std::tuple<Ts...>> : public std::tuple<std::shared_ptr<Ts>...>
    {
    };

private:
    bool areAllRequiredPresent() const
    {
        return std::get<0>(requiredData_).use_count() > 0;
        // return std::apply([](auto&... ptr) { return (... && (ptr.use_count() > 0)); },
        //                   requiredData_);
    }

    void clearAlldata()
    {
        // auto clear = [](auto&... ptr) { (..., ptr.reset()); };
        // std::apply(clear, requiredData_);
        // std::apply(clear, optionalData_);
        // std::apply([](auto&... list) { (..., list.clear()); }, listData_);
    }

private:
    Broadcasters<typename R::TupleType> requiredBroadcasters_;
    // std::tuple<Broadcaster<Type<O>>> optionalBroadcasters_;
    // std::tuple<Broadcaster<Type<L>>> listBroadcasters_;

    Data<typename R::TupleType> requiredData_;
    // std::tuple<std::shared_ptr<Type<O>>> optionalData_;
    // std::tuple<std::list<std::shared_ptr<Type<L>>>> listData_;
};

} // namespace synchro
