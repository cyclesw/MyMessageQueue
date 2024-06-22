#include "msgqueue.hpp"
#include "binding.hpp"
#include "help.hpp"
#include "host.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace rabbitMQ;

MsgQueueManagerPtr mqmp;


class ExchangeTest : public testing::Environment
{
public:
    void SetUp() override
    {
        mqmp = std::make_shared<MsgQueueManager>("./data/meta.db");
    }

    void TearDown() override
    {
        // 清理环境
        //mqmp->Clear();
    }
};

TEST(MsgQueueTest, insert)
{
    google::protobuf::Map<std::string, std::string> map;
    map.insert(std::make_pair("K1", "V1"));
    mqmp->DeclareQueue("queue1", true, false, false, map);
    mqmp->DeclareQueue("queue2", true, false, false, map);
    mqmp->DeclareQueue("queue3", true, false, false, map);
    mqmp->DeclareQueue("queue4", true, false, false, map);
    mqmp->DeclareQueue("queue5", true, false, false, map);
    EXPECT_EQ(mqmp->Size(), 5);
}

TEST(MsgQueueTest, select_test)
{
    ASSERT_EQ(mqmp->Size(), 5);
    ASSERT_EQ(mqmp->Exists("queue1"), true);
    ASSERT_EQ(mqmp->Exists("queue6"), false);
    ASSERT_EQ(mqmp->Exists("queue2"), true);
    ASSERT_EQ(mqmp->Exists("queue3"), true);
    ASSERT_EQ(mqmp->Exists("queue4"), true);
    ASSERT_EQ(mqmp->Exists("queue5"), true);

    auto mqp = mqmp->SelectQueue("queue1");
    ASSERT_NE(mqp.get(), nullptr);
    ASSERT_EQ(mqp->name, "queue1");
    ASSERT_EQ(mqp->durable, true);
    ASSERT_EQ(mqp->exclusive, false);
    ASSERT_EQ(mqp->auto_delete, false);
    ASSERT_EQ(mqp->GetArgs(), std::string("K1=V1&"));
}

TEST(MsgQueueTest, delete_test)
{
    mqmp->DeleteQueue("queue1");
    ASSERT_EQ(mqmp->Size(), 4);
    ASSERT_EQ(mqmp->Exists("queue1"), false);
    ASSERT_EQ(mqmp->Exists("queue2"), true);
    ASSERT_EQ(mqmp->Exists("queue3"), true);
    ASSERT_EQ(mqmp->Exists("queue4"), true);
    ASSERT_EQ(mqmp->Exists("queue5"), true);
}

TEST(Message, test)
{
}


int main()
{
    testing::InitGoogleTest();
    // 注册环境
    testing::AddGlobalTestEnvironment(new ExchangeTest());
    // 运行测试

    return RUN_ALL_TESTS();
}