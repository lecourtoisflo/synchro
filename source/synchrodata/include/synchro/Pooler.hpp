#pragma once

#include <boost/signals2.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace synchro
{
template<class T>
class Pooler
{
public:
    using Connection   = boost::signals2::connection;
    using CallbackType = void(std::shared_ptr<T>);
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
