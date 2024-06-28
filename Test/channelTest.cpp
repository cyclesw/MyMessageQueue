#include "threadpool.hpp"
#include "channel.hpp"
#include <gtest/gtest.h>

using namespace MyMQ;

TEST(Channel, test) {
    ChannelManagerPtr cmp = std::make_shared<ChannelManager>();

    cmp->OpenChannel("c1",
        std::make_shared<VirtualHost>("host1", "./data/host1/message/", "./data/host1/host1.db"),
        std::make_shared<ConsumerManager>(),
        ProtobufCodecPtr(),
        muduo::net::TcpConnectionPtr(),
        ThreadPool::getInstance(10));
}

int main()
{
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}