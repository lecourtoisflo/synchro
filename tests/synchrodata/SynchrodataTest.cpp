#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "synchro/Broadcaster.hpp"
#include "synchro/SynchronizedData.hpp"

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

TEST(synchrodata, data)
{
    SynchronizedData<Required<R1>, Optional<O1>> data;
    bool received     = false;
    bool received_opt = false;
    auto connection =
        data.onReceived<R1>([&received](const std::shared_ptr<R1>&) { received = true; });
    auto connection_opt =
        data.onReceived<O1>([&received_opt](const std::shared_ptr<O1>&) { received_opt = true; });

    data.send<R1>(std::make_shared<R1>());
    ASSERT_TRUE(received);

    data.send<O1>(std::make_shared<O1>());
    ASSERT_TRUE(received_opt);
}

TEST(synchrodata, waitingData)
{
    SynchronizedData<Required<R1, R2>, Optional<O1, O2>, List<L1>> data;
    bool received              = false;
    bool received2             = false;
    bool received_opt          = false;
    bool received_opt2         = false;
    unsigned int received_list = 0;
    auto connection =
        data.onReceived<R1>([&received](const std::shared_ptr<R1>&) { received = true; });
    auto connection2 =
        data.onReceived<R2>([&received2](const std::shared_ptr<R2>&) { received2 = true; });

    auto connection_opt =
        data.onReceived<O1>([&received_opt](const std::shared_ptr<O1>&) { received_opt = true; });
    auto connection_opt2 =
        data.onReceived<O2>([&received_opt2](const std::shared_ptr<O2>&) { received_opt2 = true; });

    auto connection_list =
        data.onReceived<L1>([&received_list](const std::shared_ptr<L1>&) { ++received_list; });

    data.send<O1>(std::make_shared<O1>());
    ASSERT_FALSE(received_opt); // pending
    data.send<L1>(std::make_shared<L1>());
    data.send<L1>(std::make_shared<L1>());
    ASSERT_EQ(received_list, 0); // pending

    data.send<R1>(std::make_shared<R1>());
    ASSERT_FALSE(received);
    ASSERT_FALSE(received2);
    ASSERT_FALSE(received_opt);
    ASSERT_EQ(received_list, 0);
    data.send<R2>(std::make_shared<R2>());
    ASSERT_TRUE(received);
    ASSERT_TRUE(received2);
    ASSERT_TRUE(received_opt);
    ASSERT_EQ(received_list, 2);
    data.send<L1>(std::make_shared<L1>());
    ASSERT_EQ(received_list, 3);

    data.send<O2>(std::make_shared<O2>());
    ASSERT_TRUE(received_opt2);

    received = false;
    data.send<R1>(std::make_shared<R1>());
    ASSERT_TRUE(received);
}
