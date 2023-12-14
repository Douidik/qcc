#ifndef QCC_SCAN_TEST_HPP
#define QCC_SCAN_TEST_HPP

#include "scan/scanner.hpp"
#include <gtest/gtest.h>

namespace qcc
{

TEST(Scan, Syntax_Map)
{
    EXPECT_NO_THROW(syntax_map_c89());
}

} // namespace qcc

#endif
