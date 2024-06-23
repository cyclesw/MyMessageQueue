#include "help.hpp"
#include <gtest/gtest.h>

class HelperTest : public testing::Test
{

};

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS(); 
}

// //
// // Created by lang liu on 2024/6/22.
// //

// #define BOOST_TEST_MODULE MyTestModule
// #include "help.hpp"
// #include <boost/test/included/unit_test.hpp>
// #include <boost/smart_ptr.hpp>
// #include <boost/scoped_ptr.hpp>

// BOOST_AUTO_TEST_SUITE(smart_ptr)

// BOOST_AUTO_TEST_CASE(StrHelper)
// {
// }

// BOOST_AUTO_TEST_SUITE_END()
