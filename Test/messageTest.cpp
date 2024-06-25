#include "message.hpp"
#include <gtest/gtest.h>

using namespace MyMQ;

MessageManagerPtr mmp;
MessageManagerPtr mmp1;

class messageTest : public testing::Environment
{
public:
    void SetUp() override
    {
        mmp = std::make_shared<MessageManager>("./data/message/");
        mmp->InitQueueManager("queue1");
    }

    void TearDown() override
    {
        mmp->Clear();
    }
};


TEST(MessageManager, insert)
{
    BasicProperties properties;
    properties.set_id(UUIDHelper::UUID());
    properties.set_delivery_mode(DeliveryMode::DURABLE);
    properties.set_routing_key("news.music.pop");
    mmp->Insert("queue1", &properties, "Hello World-1");
    mmp->Insert("queue1", nullptr, "Hello World-2", true);
    mmp->Insert("queue1", nullptr, "Hello World-3", true);
    mmp->Insert("queue1", nullptr, "Hello World-4", true);
    mmp->Insert("queue1", nullptr, "Hello World-5", false);

    mmp->InitQueueManager("queue2");
    mmp->Insert("queue2", &properties, "Hello World-1");
    mmp->Insert("queue2", nullptr, "Hello World-2", true);
    mmp->Insert("queue2", nullptr, "Hello World-3", false);
    mmp->Insert("queue2", nullptr, "Hello World-4", true);
    mmp->Insert("queue2", nullptr, "Hello World-5", true);


    EXPECT_EQ(mmp->GetTableCount("queue1"), 5);
    EXPECT_EQ(mmp->GetTotalCount("queue1"), 4);
    EXPECT_EQ(mmp->GetDurableCount("queue1"), 4);
    EXPECT_EQ(mmp->GetWaitackCount("queue1"), 0);

    EXPECT_EQ(mmp->GetTableCount("queue2"), 5);
    EXPECT_EQ(mmp->GetTotalCount("queue2"), 4);
    EXPECT_EQ(mmp->GetDurableCount("queue2"), 4);
    EXPECT_EQ(mmp->GetWaitackCount("queue2"), 0);
}

TEST(MessageManager, selectQ1)
{
    ASSERT_EQ(mmp->GetTableCount("queue1"), 5);
    MessagePtr msg = mmp->Front("queue1");
    ASSERT_NE(msg.get(), nullptr);
    ASSERT_EQ(msg->payload().body(), std::string("Hello World-1"));
    ASSERT_EQ(mmp->GetTableCount("queue1"), 4);
    ASSERT_EQ(mmp->GetWaitackCount("queue1"), 1);

    auto msg2 = mmp->Front("queue1");
    ASSERT_NE(msg2.get(), nullptr);
    ASSERT_EQ(msg2->payload().body(), std::string("Hello World-2"));
    ASSERT_EQ(mmp->GetTableCount("queue1"), 3);
    ASSERT_EQ(mmp->GetWaitackCount("queue1"), 2);

    auto msg3 = mmp->Front("queue1");
    ASSERT_NE(msg3.get(), nullptr);
    ASSERT_EQ(msg3->payload().body(), std::string("Hello World-3"));
    ASSERT_EQ(mmp->GetTableCount("queue1"), 2);
    ASSERT_EQ(mmp->GetWaitackCount("queue1"), 3);

    // auto msg4 = mmp->Front("queue1");
    // ASSERT_NE(msg4.get(), nullptr);
    // ASSERT_EQ(msg4->payload().body(), std::string("Hello World-4"));
    // ASSERT_EQ(mmp->GetTableCount("queue1"), 1);
    // ASSERT_EQ(mmp->GetWaitackCount("queue1"), 4);

    // auto msg5 = mmp->Front("queue1");
    // ASSERT_NE(msg5.get(), nullptr);
    // ASSERT_EQ(msg4->payload().body(), std::string("Hello World-5"));
    // ASSERT_EQ(mmp->GetTableCount("queue1"), 0);
    // ASSERT_EQ(mmp->GetWaitackCount("queue1"), 5);
}

TEST(MessageManager, deleteTest)
{
    auto msg1 = mmp->Front("queue1");
    ASSERT_EQ(mmp->GetTableCount("queue1"), 1);
    ASSERT_EQ(mmp->GetWaitackCount("queue1"), 4);
    ASSERT_EQ(mmp->GetTotalCount("queue1"), 4);

    ASSERT_NE(msg1.get(), nullptr);
    ASSERT_EQ(msg1->payload().body(), std::string("Hello World-4"));
    mmp->Ack("queue1", msg1->payload().properties().id());
    ASSERT_EQ(mmp->GetWaitackCount("queue1"), 3);
    ASSERT_EQ(mmp->GetTotalCount("queue1"), 4);
}

TEST(MessageManager, Destory)
{
    mmp->DestroyQueueMessage("queue1");
}


int main()
{
    testing::InitGoogleTest();
    testing::AddGlobalTestEnvironment(new messageTest);
    system("pwd");
    

    return RUN_ALL_TESTS();
}