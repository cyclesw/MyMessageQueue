#include "binding.hpp"
#include <gtest/gtest.h>

using namespace rabbitMQ;

BindingManagerPtr bmp;

class BindingTest : public testing::Environment 
{
public:
    void SetUp() override
    {
        bmp = std::make_shared<BindingManager>("./data/meta.db");
    }

    void TearDown() override
    {
        // bmp->Clear();
    }
};

TEST(BindingTest, insert)
{
    bmp->Bind("exchange1", "queue1", "news.music.#", true);
    bmp->Bind("exchange1", "queue2", "news.sport.#", true);
    bmp->Bind("exchange1", "queue3", "news.gossip.#", true);
    bmp->Bind("exchange2", "queue1", "news.music.pop", true);
    bmp->Bind("exchange2", "queue2", "news.sport.football", true);
    bmp->Bind("exchange2", "queue3", "news.gossip.#", true);

    ASSERT_EQ(bmp->Size(), 6);
}

TEST(BindingTest, select)
{
    EXPECT_EQ(bmp->Exists("exchange1", "queue1"), true);
    EXPECT_EQ(bmp->Exists("exchange1", "queue2"), true);
    EXPECT_EQ(bmp->Exists("exchange1", "queue3"), true);
    EXPECT_EQ(bmp->Exists("exchange2", "queue1"), true);
    EXPECT_EQ(bmp->Exists("exchange2", "queue2"), true);
    EXPECT_EQ(bmp->Exists("exchange2", "queue3"), true);
}

TEST(BindingTest, remove)
{
    bmp->RemoveExchangeBindings("exchange1");
    EXPECT_EQ(bmp->Exists("exchange1", "queue1"), false);
    EXPECT_EQ(bmp->Exists("exchange1", "queue2"), false);
    EXPECT_EQ(bmp->Exists("exchange1", "queue3"), false);
}

TEST(BindTest, UnBind)
{
    EXPECT_EQ(bmp->Exists("exchange2", "queue2"), true);
    bmp->UnBind("exchange2", "queue2");
    EXPECT_EQ(bmp->Exists("exchange2", "queue2"), false);
    EXPECT_EQ(bmp->Exists("exchange2", "queue3"), true);
}

int main()
{
    testing::InitGoogleTest();
    testing::AddGlobalTestEnvironment(new BindingTest);

    return RUN_ALL_TESTS();
}