#include <gtest/gtest.h>
#include "channel.hpp"

using namespace MyMQ;

class ChannelTest : public testing::Test
{
public:
    void SetUp() override
    {
        vhp = std::make_shared<VirtualHost>("host1", "./data/message", "./data/dbfile");
        cmp = std::make_shared<ChannelManager>();
    }

    void TearDown() override;

    VirtualHostPtr vhp;
    ChannelManagerPtr cmp;
};

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}