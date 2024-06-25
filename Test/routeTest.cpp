//
// Created by lang liu on 2024/6/22.
//

#include "route.hpp"
#include <gtest/gtest.h>
using namespace MyMQ;

TEST(router, IsLegal)
{
    ASSERT_EQ(Router::IsLegalRoutingKey("news.music.pop"), true);
    ASSERT_EQ(Router::IsLegalRoutingKey("news.music.."), true);
    ASSERT_NE(Router::IsLegalRoutingKey("news.music.~'"), true);

    ASSERT_EQ(Router::IsLegalBindingKey("news.music."), true);
    ASSERT_EQ(Router::IsLegalBindingKey("news.."), true);  //TODO 鉴定
    ASSERT_EQ(Router::IsLegalBindingKey("news.#"), true);
    ASSERT_EQ(Router::IsLegalBindingKey("news#"), false);
    ASSERT_EQ(Router::IsLegalBindingKey("news#."), false);
    ASSERT_EQ(Router::IsLegalBindingKey("news.music*"), false);
    ASSERT_EQ(Router::IsLegalBindingKey("news.music#"), false);
    ASSERT_EQ(Router::IsLegalBindingKey("news.music.*#"), false);
    ASSERT_EQ(Router::IsLegalBindingKey("news.music.*"), true);
}

TEST(router, DirectRoute)
{
    ASSERT_EQ(Router::Route(ExchangeType::DIRECT, "aaa", "aaa"), true);
    ASSERT_EQ(Router::Route(ExchangeType::DIRECT, "aaa.bbb", "aaa.bbb"), true);
    ASSERT_EQ(Router::Route(ExchangeType::DIRECT, "aaa.bbb", "bbb.aaa"), false);
}

TEST(RouterTest, FanoutExchange)
{
    ASSERT_EQ(Router::Route(ExchangeType::FANOUT, "aaa", "anything"), true);
    ASSERT_EQ(Router::Route(ExchangeType::FANOUT, "bbb", "anything"), true);
}

TEST(RouterTest, TopicExchange)
{
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa", "aaa"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb", "aaa.bbb"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc", "aaa.bbb.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc", "aaa.*.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc", "aaa.#"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "bbb.ccc", "#.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "ccc", "#.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc", "#.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc.ccc.ccc", "aaa.#.ccc.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc", "aaa.#.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc", "aaa.#.bbb"), false);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.ccc", "aaa.#.bbb"), false);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb", "aaa.*"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc.ddd", "aaa.#"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.aaa.bbb.ccc", "*.aaa.bbb.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.bbb.ccc.ccc.ccc", "aaa.#.ccc.ccc"), true);
    ASSERT_EQ(Router::Route(ExchangeType::TOPIC, "aaa.ddd.ccc.bbb.eee.bbb", "aaa.#.bbb.*.bbb"), true);
}

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}