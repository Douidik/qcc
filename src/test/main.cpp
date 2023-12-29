#include "regex_test.hpp"
#include "scan_test.hpp"
#include "x86_test.hpp"
//
#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
