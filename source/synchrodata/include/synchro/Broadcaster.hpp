#pragma once

#include <boost/signals2.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace synchro
{
/**
 * @brief Handler for signal notifications
 */
template<class T>
class Broadcaster
{
public:
    using Connection   = boost::signals2::connection;     ///< Notification connection
    using CallbackType = void(const std::shared_ptr<T>&); ///< Callback prototype for notification
    using Callback     = std::function<CallbackType>;     ///< Callback for notification

public:
    /**
     * @brief Register notification callback
     * @param cbk callback for notification
     * @returns Connection of the notification
     */
    Connection onReceived(Callback&& cbk) { return signal_.connect(std::move(cbk)); }

    /**
     * @brief Send an element to broadcast
     * @param data element to broadcast to registered callbacks
     */
    void send(const std::shared_ptr<T>& data) { signal_(data); }

private:
    using Signal = boost::signals2::signal<CallbackType>;

private:
    Signal signal_;
};
} // namespace synchro
