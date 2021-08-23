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

template<class R, class O = Optional<>, class L = List<>>
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
        else if constexpr (Contains<T, typename O::TupleType>())
        {
            return std::get<Broadcaster<T>>(optionalBroadcasters_)
                .onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
        else if constexpr (Contains<T, typename L::TupleType>())
        {
            return std::get<Broadcaster<T>>(listBroadcasters_)
                .onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
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
                initDone_ = true;
            }
        }
        else if constexpr (Contains<T, typename O::TupleType>())
        {
            if (initDone_)
            {
                std::get<Broadcaster<T>>(optionalBroadcasters_).send(data);
                return;
            }
            std::get<std::shared_ptr<T>>(optionalData_) = data;
        }
        else if constexpr (Contains<T, typename L::TupleType>())
        {
            if (initDone_)
            {
                std::get<Broadcaster<T>>(requiredBroadcasters_).send(data);
                return;
            }
            std::get<std::list<std::shared_ptr<T>>>(listData_).push_back(data);
        }
    }

private:
    template<class T>
    struct Broadcasters;
    template<class... Ts>
    struct Broadcasters<std::tuple<Ts...>> : std::tuple<Broadcaster<Ts>...>
    {
        using type = std::tuple<Broadcaster<Ts>...>;
    };
    template<class T>
    struct Data;
    template<class... Ts>
    struct Data<std::tuple<Ts...>>
    {
        using type = std::tuple<std::shared_ptr<Ts>...>;
    };

    template<class T>
    struct DataList;
    template<class... Ts>
    struct DataList<std::tuple<Ts...>>
    {
        template<class T>
        using element = std::list<std::shared_ptr<T>>;
        using type    = std::tuple<element<Ts>...>;
    };

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
    bool initDone_ = false;
    typename Broadcasters<typename R::TupleType>::type requiredBroadcasters_;
    typename Broadcasters<typename O::TupleType>::type optionalBroadcasters_;
    typename Broadcasters<typename L::TupleType>::type listBroadcasters_;

    typename Data<typename R::TupleType>::type requiredData_;
    typename Data<typename O::TupleType>::type optionalData_;
    typename DataList<typename L::TupleType>::type listData_;
};

} // namespace synchro
