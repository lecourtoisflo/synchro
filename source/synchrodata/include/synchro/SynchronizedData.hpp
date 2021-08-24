#pragma once

#include "Broadcaster.hpp"

#include <atomic>
#include <list>
#include <memory>
#include <tuple>
#include <type_traits>

namespace synchro
{

/// @brief Trait class to define required type for synchronized data
template<class... Ts>
struct Required
{
    using TupleType = std::tuple<Ts...>; ///< tuple type for required data
};

/// @brief Trait class to define optional type for synchronized data
template<class... Ts>
struct Optional
{
    using TupleType = std::tuple<Ts...>; ///< tuple type for optional data
};

/// @brief Trait class to define listed type for synchronized data
template<class... Ts>
struct List
{
    using TupleType = std::tuple<Ts...>; ///< tuple type for listed data
};

/**
 * @brief Synchronized data
 *
 * Synchronized data allows to synchronize notifications when receiving data according to whether
 * required data are received:
 * - R : required types (based on @a Required trait). Synchronized data contains 1 element by
 * required type. No notification is sent unless at least one required data of each type is
 * received. In the case multiple required elements of the same type are received before full
 * synchronization is achieved, only the last one will be sent.
 * - O : optional types (based on @a Optional trait). Synchronized data contains at most 1 element
 * by optional type. No notification is sent unless at least one required data of each type is
 * received. In the case multiple optional elements of the same type are received before full
 * synchronization is achieved, only the last one will be sent.
 * - L : listed types (based on @a List trait). Synchronized data contains n elements by listed
 * type. No notification is sent unless at least one required data of each type is received. A
 * notification will be sent for each element of each list type.
 */
template<class R, class O = Optional<>, class L = List<>>
class SynchronizedData
{
public:
    /// @brief Connection type by element
    template<class T>
    using Connection = typename Broadcaster<T>::Connection;

public:
    /**
     * @brief Register callback for type T
     *
     * @param cbk callback with prototype void(const std::shared_ptr<T>&) to received notification
     * @returns broadcaster connection to store
     */
    template<class T>
    Connection<T> onReceived(typename Broadcaster<T>::Callback&& cbk)
    {
        if constexpr (Contains<T, typename R::TupleType>())
        {
            return std::get<Broadcaster<T>>(requiredBroadcasters_).onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
        else if constexpr (Contains<T, typename O::TupleType>())
        {
            return std::get<Broadcaster<T>>(optionalBroadcasters_).onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
        else if constexpr (Contains<T, typename L::TupleType>())
        {
            return std::get<Broadcaster<T>>(listBroadcasters_).onReceived(std::forward<typename Broadcaster<T>::Callback>(cbk));
        }
        return Connection<T>();
    }

    /**
     * @brief Send a data element
     *
     * does nothing if element is not in the defined types of the synchronized data
     *
     * @param data the element to send
     */
    template<class T>
    void send(const std::shared_ptr<T>& data)
    {
        if constexpr (Contains<T, typename R::TupleType>())
        {
            std::get<std::shared_ptr<T>>(requiredPendingData_) = data; // required for following test
            if (initDone_ || areAllRequiredPresent())
            {
                std::get<Broadcaster<T>>(requiredBroadcasters_).send(data);
                std::get<std::shared_ptr<T>>(requiredPendingData_).reset();

                // send pending signals
                sendSignalsRequired(requiredBroadcasters_, requiredPendingData_);
                sendSignalsOptional(optionalBroadcasters_, optionalPendingData_);
                sendSignalsList(listBroadcasters_, listPendingData_);

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
            std::get<std::shared_ptr<T>>(optionalPendingData_) = data; // put in pending
        }
        else if constexpr (Contains<T, typename L::TupleType>())
        {
            if (initDone_)
            {
                std::get<Broadcaster<T>>(listBroadcasters_).send(data);
                return;
            }
            std::get<std::list<std::shared_ptr<T>>>(listPendingData_).push_back(data); // put in pending
        }
    }

    /// @brief Clear all broadcasters and pending data
    void clear()
    {
        clearAlldata();
        auto clear = [](auto&... broadcaster) { (..., broadcaster.clear()); };
        std::apply(clear, requiredBroadcasters_);
        std::apply(clear, optionalBroadcasters_);
        std::apply(clear, listBroadcasters_);
        initDone_ = false;
    }

private:
    /// @brief Trait class to check if a type T is contained in the tuple Tuple
    template<class T, class Tuple>
    struct Contains;
    /// @brief Specialization for tuple
    template<class T, class... Ts>
    struct Contains<T, std::tuple<Ts...>> : std::disjunction<std::is_same<T, Ts>...>
    {
    };

    /// @brief Broadcasters trait class to define tuple of broadcasters
    template<class T>
    struct Broadcasters;
    /// @brief Specialization to define a tuple of broadcaster<T> from tuple of T
    template<class... Ts>
    struct Broadcasters<std::tuple<Ts...>>
    {
        using type = std::tuple<Broadcaster<Ts>...>;
    };

    /// @brief Data trait class to define tuple of elements
    template<class T>
    struct Data;
    /// @brief Specialization to define a tuple of shared pointers from a tuple
    template<class... Ts>
    struct Data<std::tuple<Ts...>>
    {
        using type = std::tuple<std::shared_ptr<Ts>...>;
    };

    /// @brief Data trait class to define tuple of lists of elements
    template<class T>
    struct DataList;
    /// @brief Specialization to define a tuple of lists of shared pointers from a tuple
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
        return std::apply([](auto&... ptr) { return (... && (ptr.use_count() > 0)); }, requiredPendingData_);
    }

    void clearAlldata()
    {
        auto clear = [](auto&... ptr) { (..., ptr.reset()); };
        std::apply(clear, requiredPendingData_);
        std::apply(clear, optionalPendingData_);
        std::apply([](auto&... list) { (..., list.clear()); }, listPendingData_);
    }
    template<class T>
    static void sendSignal(Broadcaster<T>& sender, const std::shared_ptr<T>& data)
    {
        if (data) { sender.send(data); }
    }

    template<class... Ts>
    void sendSignalsRequired(std::tuple<Broadcaster<Ts>...>&, const std::tuple<std::shared_ptr<Ts>...>&)
    {
        static_assert(std::tuple_size_v<std::tuple<Broadcaster<Ts>...>> == std::tuple_size_v<std::tuple<std::shared_ptr<Ts>...>>);
        sendSignalsRequiredImpl<std::tuple_size_v<std::tuple<Broadcaster<Ts>...>>>();
    }

    template<size_t I, std::enable_if_t<(I > 0), bool> = true>
    void sendSignalsRequiredImpl()
    {
        sendSignal(std::get<I - 1>(requiredBroadcasters_), std::get<I - 1>(requiredPendingData_));
        sendSignalsRequiredImpl<I - 1>();
    }

    template<size_t I, std::enable_if_t<(I == 0), bool> = true>
    void sendSignalsRequiredImpl()
    {
    }

    template<class... Ts>
    void sendSignalsOptional(std::tuple<Broadcaster<Ts>...>&, const std::tuple<std::shared_ptr<Ts>...>&)
    {
        static_assert(std::tuple_size_v<std::tuple<Broadcaster<Ts>...>> == std::tuple_size_v<std::tuple<std::shared_ptr<Ts>...>>);
        sendSignalsOptionalImpl<std::tuple_size_v<std::tuple<Broadcaster<Ts>...>>>();
    }

    template<size_t I, std::enable_if_t<(I > 0), bool> = true>
    void sendSignalsOptionalImpl()
    {
        sendSignal(std::get<I - 1>(optionalBroadcasters_), std::get<I - 1>(optionalPendingData_));
        sendSignalsOptionalImpl<I - 1>();
    }

    template<size_t I, std::enable_if_t<(I == 0), bool> = true>
    void sendSignalsOptionalImpl()
    {
    }

    template<class... Ts>
    void sendSignalsList(std::tuple<Broadcaster<Ts>...>&, const std::tuple<std::list<std::shared_ptr<Ts>>...>&)
    {
        static_assert(std::tuple_size_v<std::tuple<Broadcaster<Ts>...>> == std::tuple_size_v<std::tuple<std::list<std::shared_ptr<Ts>>...>>);
        sendSignalsListImpl<std::tuple_size_v<std::tuple<Broadcaster<Ts>...>>>();
    }

    template<size_t I, std::enable_if_t<(I > 0), bool> = true>
    void sendSignalsListImpl()
    {
        for (const auto& data : std::get<I - 1>(listPendingData_))
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
    std::atomic_bool initDone_ = false;
    BroadcastersTuple<R> requiredBroadcasters_;
    BroadcastersTuple<O> optionalBroadcasters_;
    BroadcastersTuple<L> listBroadcasters_;

    DataTuple<R> requiredPendingData_;
    DataTuple<O> optionalPendingData_;
    DataListTuple<L> listPendingData_;
};

} // namespace synchro
