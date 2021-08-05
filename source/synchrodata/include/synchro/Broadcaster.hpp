#pragma once

#include <boost/signals2.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace synchro
{
template<class T>
class Broadcaster
{
public:
    using Connection   = boost::signals2::connection;
    using CallbackType = void(const std::shared_ptr<T>&);
    using Callback     = std::function<CallbackType>;

public:
    Connection onReceived(Callback&& cbk) { return signal_.connect(std::move(cbk)); }
    void send(const std::shared_ptr<T>& data) { signal_(data); }

private:
    using Signal = boost::signals2::signal<CallbackType>;

private:
    Signal signal_;
};
} // namespace synchro
