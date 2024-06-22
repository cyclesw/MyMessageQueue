//
// Created by lang liu on 2024/6/22.
//
#include "host.hpp"
#include <gtest/gtest.h>

using namespace rabbitMQ;

class VirtualHostTest : public testing::Test
{
public:
    void SetUp() override
    {
        LOG_INFO("开始初始化");
        google::protobuf::Map<std::string, std::string> empty;
        _host = std::make_shared<VirtualHost>("host1", "./data/host1/message/", "./data/host/host1.db");
        _host->DeclareExchange("exchange1", ExchangeType::DIRECT, true, false, empty);
        _host->DeclareExchange("exchange2", ExchangeType::DIRECT, true, false, empty);
        _host->DeclareExchange("exchange3", ExchangeType::DIRECT, true, false, empty);

        _host->DeclareQueue("queue1", true, false, true, empty);
        _host->DeclareQueue("queue2", true, false, true, empty);
        _host->DeclareQueue("queue3", true, false, true, empty);

        _host->Bind("exchange1", "queue1", "news.music.#");
        _host->Bind("exchange1", "queue2", "news.music.#");
        _host->Bind("exchange1", "queue3", "news.music.#");

        _host->Bind("exchange2", "queue1", "news.music.#");
        _host->Bind("exchange2", "queue2", "news.music.#");
        _host->Bind("exchange2", "queue3", "news.music.#");

        _host->Bind("exchange3", "queue1", "news.music.#");
        _host->Bind("exchange3", "queue2", "news.music.#");
        _host->Bind("exchange3", "queue3", "news.music.#");

        _host->BasicPublish("queue1", nullptr, "Hello-World1");
        _host->BasicPublish("queue1", nullptr, "Hello-World2");
        _host->BasicPublish("queue1", nullptr, "Hello-World3");

        _host->BasicPublish("queue2", nullptr, "Hello-World1");
        _host->BasicPublish("queue2", nullptr, "Hello-World2");
        _host->BasicPublish("queue2", nullptr, "Hello-World3");

        _host->BasicPublish("queue3", nullptr, "Hello-World1");
        _host->BasicPublish("queue3", nullptr, "Hello-World2");
        _host->BasicPublish("queue3", nullptr, "Hello-World3");
    }

    void TearDown() override
    {
        _host->Clear();
    }
public:
    VirtualHostPtr _host;
};

TEST_F(VirtualHostTest, InitTest)
{
    ASSERT_EQ(_host->ExistExchange("exchange1"), true);
    ASSERT_EQ(_host->ExistExchange("exchange2"), true);
    ASSERT_EQ(_host->ExistExchange("exchange3"), true);

    ASSERT_EQ(_host->ExistQueue("queue1"), true);
    ASSERT_EQ(_host->ExistQueue("queue2"), true);
    ASSERT_EQ(_host->ExistQueue("queue3"), true);

    ASSERT_EQ(_host->ExistBinding("exchange1", "queue1"), true);
    ASSERT_EQ(_host->ExistBinding("exchange1", "queue2"), true);
    ASSERT_EQ(_host->ExistBinding("exchange1", "queue3"), true);

    ASSERT_EQ(_host->ExistBinding("exchange2", "queue1"), true);
    ASSERT_EQ(_host->ExistBinding("exchange2", "queue2"), true);
    ASSERT_EQ(_host->ExistBinding("exchange2", "queue3"), true);

    ASSERT_EQ(_host->ExistBinding("exchange3", "queue1"), true);
    ASSERT_EQ(_host->ExistBinding("exchange3", "queue2"), true);
    ASSERT_EQ(_host->ExistBinding("exchange3", "queue3"), true);
}

//TEST_F(VirtualHostTest, RemoveTest)
//{
////    _host->DeleteExchange("exchange1");
//
//}

int main()
{
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}