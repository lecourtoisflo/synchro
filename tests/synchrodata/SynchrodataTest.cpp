#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "synchro/Broadcaster.hpp"
#include "synchro/SynchronizedData.hpp"

#include <iostream>
#include <typeindex>
#include <typeinfo>

using namespace synchro;

TEST(synchrodata, broadcast)
{
    Broadcaster<int> Broadcaster;

    constexpr int value  = 5;
    unsigned int counter = 0;

    auto recv = [&value, &counter](const std::shared_ptr<int>& data)
    {
        counter++;
        ASSERT_EQ(*data, value);
    };

    auto connection = Broadcaster.onReceived(recv);
    Broadcaster.send(std::make_shared<int>(value));
    ASSERT_EQ(counter, 1);
    connection.disconnect();
    Broadcaster.send(std::make_shared<int>(value));
    ASSERT_EQ(counter, 1);
}

TEST(synchrodata, data)
{
    struct R1
    {
    };
    struct R2
    {
    };
    struct O1
    {
    };
    struct O2
    {
    };
    struct L1
    {
    };
    struct L2
    {
    };
    SynchronizedData<Required<R1>, Optional<O1>, List<L1>> data;

    bool received = false;
    auto connection =
        data.onReceived<R1>([&received](const std::shared_ptr<R1>&) { received = true; });

    data.send<R1>(std::make_shared<R1>());
    ASSERT_TRUE(received);
}