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
            std::get<std::shared_ptr<T>>(requiredData_) = data; // required for following test
            if (areAllRequiredPresent())
            {
                std::get<Broadcaster<T>>(requiredBroadcasters_).send(data);
                std::get<std::shared_ptr<T>>(requiredData_).reset();

                // send pending signals
                sendSignalsRequired(requiredBroadcasters_, requiredData_);
                sendSignalsOptional(optionalBroadcasters_, optionalData_);
                sendSignalsList(listBroadcasters_, listData_);

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
            std::get<std::shared_ptr<T>>(optionalData_) = data; // put in pending
        }
        else if constexpr (Contains<T, typename L::TupleType>())
        {
            if (initDone_)
            {
                std::get<Broadcaster<T>>(listBroadcasters_).send(data);
                return;
            }
            std::get<std::list<std::shared_ptr<T>>>(listData_).push_back(data); // put in pending
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

    template<class T>
    using BroadcastersTuple = typename Broadcasters<typename T::TupleType>::type;
    template<class T>
    using DataTuple = typename Data<typename T::TupleType>::type;
    template<class T>
    using DataListTuple = typename DataList<typename T::TupleType>::type;

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
    template<class T>
    static void sendSignal(Broadcaster<T>& sender, const std::shared_ptr<T>& data)
    {
        if (data) { sender.send(data); }
    }

    template<class... Ts>
    void sendSignalsRequired(std::tuple<Broadcaster<Ts>...>&,
                             const std::tuple<std::shared_ptr<Ts>...>&)
    {
        static_assert(std::tuple_size_v<std::tuple<Broadcaster<Ts>...>> ==
                      std::tuple_size_v<std::tuple<std::shared_ptr<Ts>...>>);
        sendSignalsRequiredImpl<std::tuple_size_v<std::tuple<Broadcaster<Ts>...>>>();
    }

    template<size_t I, std::enable_if_t<(I > 0), bool> = true>
    void sendSignalsRequiredImpl()
    {
        sendSignal(std::get<I - 1>(requiredBroadcasters_), std::get<I - 1>(requiredData_));
        sendSignalsRequiredImpl<I - 1>();
    }

    template<size_t I, std::enable_if_t<(I == 0), bool> = true>
    void sendSignalsRequiredImpl()
    {
    }

    template<class... Ts>
    void sendSignalsOptional(std::tuple<Broadcaster<Ts>...>&,
                             const std::tuple<std::shared_ptr<Ts>...>&)
    {
        static_assert(std::tuple_size_v<std::tuple<Broadcaster<Ts>...>> ==
                      std::tuple_size_v<std::tuple<std::shared_ptr<Ts>...>>);
        sendSignalsOptionalImpl<std::tuple_size_v<std::tuple<Broadcaster<Ts>...>>>();
    }

    template<size_t I, std::enable_if_t<(I > 0), bool> = true>
    void sendSignalsOptionalImpl()
    {
        sendSignal(std::get<I - 1>(optionalBroadcasters_), std::get<I - 1>(optionalData_));
        sendSignalsOptionalImpl<I - 1>();
    }

    template<size_t I, std::enable_if_t<(I == 0), bool> = true>
    void sendSignalsOptionalImpl()
    {
    }

    template<class... Ts>
    void sendSignalsList(std::tuple<Broadcaster<Ts>...>&,
                         const std::tuple<std::list<std::shared_ptr<Ts>>...>&)
    {
        static_assert(std::tuple_size_v<std::tuple<Broadcaster<Ts>...>> ==
                      std::tuple_size_v<std::tuple<std::list<std::shared_ptr<Ts>>...>>);
        sendSignalsListImpl<std::tuple_size_v<std::tuple<Broadcaster<Ts>...>>>();
    }

    template<size_t I, std::enable_if_t<(I > 0), bool> = true>
    void sendSignalsListImpl()
    {
        for (const auto& data : std::get<I - 1>(listData_))
        {
            sendSignal(std::get<I - 1>(listBroadcasters_), data);
        }
        sendSignalsListImpl<I - 1>();
    }

    template<size_t I, std::enable_if_t<(I == 0), bool> = true>
    void sendSignalsListImpl()
    {
    }

private:
    bool initDone_ = false;
    BroadcastersTuple<R> requiredBroadcasters_;
    BroadcastersTuple<O> optionalBroadcasters_;
    BroadcastersTuple<L> listBroadcasters_;

    DataTuple<R> requiredData_;
    DataTuple<O> optionalData_;
    DataListTuple<L> listData_;
};

} // namespace synchro
