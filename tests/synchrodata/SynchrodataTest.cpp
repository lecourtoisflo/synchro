#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "synchro/Broadcaster.hpp"

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