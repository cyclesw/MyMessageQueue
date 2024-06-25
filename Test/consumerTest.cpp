//
// Created by lang liu on 2024/6/23.
//

#include <gtest/gtest.h>
#include "consumer.hpp"

using namespace MyMQ;

class ConsumerTest : public testing::Test
{
public:
    void SetUp() override
    {
        LOG_INFO("初始化ConsumerTest");
        cmp = std::make_shared<ConsumerManager>();
        cmp->InitQueueConsumer("queue1");
    }

    void TearDown() override
    {
        cmp->Clear();
    }

    ConsumerManagerPtr cmp;
};

void func(const std::string &tag, const BasicProperties *bp, const std::string &body)
{
    std::cout << "消费信息：" << body;
}

TEST_F(ConsumerTest, insert)
{
    cmp->Create("c1", "queue1", false, func);
    cmp->Create("c2", "queue1", false, func);
    cmp->Create("c3", "queue1", false, func);

    ASSERT_EQ(cmp->Exists("c1", "queue1"), true);
    ASSERT_EQ(cmp->Exists("c2", "queue1"), true);
    ASSERT_EQ(cmp->Exists("c3", "queue1"), true);
}

TEST_F(ConsumerTest, choose)
{
    cmp->Create("c1", "queue1", false, func);
    cmp->Create("c2", "queue1", false, func);
    cmp->Create("c3", "queue1", false, func);

    auto cp = cmp->Choose("queue1");
    ASSERT_NE(cp.get(), nullptr);
    ASSERT_EQ(cp->tag, "c1");

    cp = cmp->Choose("queue1");
    ASSERT_NE(cp.get(), nullptr);
    ASSERT_EQ(cp->tag, "c2");

    cp = cmp->Choose("queue1");
    ASSERT_NE(cp.get(), nullptr);
    ASSERT_EQ(cp->tag, "c3");
}

TEST_F(ConsumerTest, remove)
{
    cmp->Create("c1", "queue1", false, func);
    cmp->Create("c2", "queue1", false, func);
    cmp->Create("c3", "queue1", false, func);

    cmp->Remove("c1", "queue1");
    ASSERT_EQ(cmp->Exists("c1", "queue1"), false);

    cmp->Remove("c2", "queue1");
    ASSERT_EQ(cmp->Exists("c2", "queue1"), false);

    cmp->Remove("c3", "queue1");
    ASSERT_EQ(cmp->Exists("c3", "queue1"), false);
}

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}