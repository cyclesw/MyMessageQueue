#include "threadpool.hpp"
#include "channel.hpp"
#include <gtest/gtest.h>

using namespace MyMQ;

// class ChannelTest : public testing::Test
// {
// public:
//     void SetUp() override
//     {
//         vhp = std::make_shared<VirtualHost>("host1", "./data/message", "./data/dbfile");
//         cmp = std::make_shared<ChannelManager>();
//     }

//     void TearDown() override;

//     VirtualHostPtr vhp;
//     ChannelManagerPtr cmp;
// };

//[x]
TEST(ChannelTest, DeclareExchange)
{
    ChannelManagerPtr cmp;
    auto host = std::make_shared<VirtualHost>("host1", "./data/message", "./data/message/host1.db");
    auto consumer = std::make_shared<ConsumerManager>();
    cmp->OpenChannel("c1", host, consumer, ProtobufCodecPtr(),
            muduo::net::TcpConnectionPtr(), ThreadPool::getInstance(5));
}

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}