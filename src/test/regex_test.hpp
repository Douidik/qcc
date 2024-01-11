#ifndef QCC_REGEX_TEST_HPP
#define QCC_REGEX_TEST_HPP

#include "regex.hpp"
#include "regex/format.hpp"
#include <gtest/gtest.h>

namespace qcc::regex
{

using namespace std::string_view_literals;

constexpr std::string_view LOREM_IPSUM = R"(
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut
labore et dolore magna aliqua. Id porta nibh venenatis cras sed felis eget velit. Viverra tellus
in hac habitasse. Sed risus pretium quam vulputate dignissim suspendisse in est. In eu mi
bibendum neque egestas congue quisque egestas. Mi proin sed libero enim sed faucibus turpis in.
Aliquam vestibulum morbi blandit cursus. Tellus in hac habitasse platea dictumst vestibulum.
Massa ultricies mi quis hendrerit. Molestie a iaculis at erat pellentesque adipiscing commodo.
Vulputate eu scelerisque felis imperdiet proin fermentum. Vitae congue eu consequat ac felis. Nec
ultrices dui sapien eget mi proin sed. Nunc mattis enim ut tellus elementum sagittis vitae et.
Mauris ultrices eros in cursus turpis massa tincidunt dui ut. Nisi porta lorem mollis aliquam ut
porttitor leo a diam. Diam phasellus vestibulum lorem sed risus ultricies. Arcu vitae elementum
curabitur vitae nunc sed velit dignissim. Ut eu sem integer vitae justo eget magna fermentum
iaculis.In eu mi bibendum neque.
)";

inline std::string quoted(std::string_view expression)
{
    return fmt::format("'{}'", expression);
}

TEST(Regex, Unknown_Token)
{
    EXPECT_THROW("N"_rx, Error);
    EXPECT_THROW(")"_rx, Error);
    EXPECT_THROW("ù"_rx, Error);
}

TEST(Regex, Text)
{
    EXPECT_TRUE("'abc'"_rx.match("abc"));
    EXPECT_TRUE("'abc'"_rx.match("abcccccccccc"));
    EXPECT_TRUE("'hello' ' ' 'world'"_rx.match("hello world"));
    EXPECT_TRUE("'hello\nworld'"_rx.match("hello\nworld"));
    EXPECT_TRUE(Regex(quoted(LOREM_IPSUM)).match(LOREM_IPSUM));

    EXPECT_TRUE("`abc`"_rx.match("abc"));
    EXPECT_TRUE("`abc`"_rx.match("abcccccccccc"));
    EXPECT_TRUE("`hello` ` ` `world`"_rx.match("hello world"));
    EXPECT_TRUE("`hello\nworld`"_rx.match("hello\nworld"));

    EXPECT_THROW("`hello'"_rx, Error);
    EXPECT_THROW("'hello`"_rx, Error);
    EXPECT_THROW("'hello"_rx, Error);
    EXPECT_THROW("hello'"_rx, Error);
    EXPECT_THROW("hello`"_rx, Error);
    EXPECT_THROW("`hello"_rx, Error);
    EXPECT_THROW("hello"_rx, Error);

    EXPECT_FALSE("'cba'"_rx.match("abc"));
    EXPECT_FALSE("'cbaa'"_rx.match("abcc"));
    EXPECT_FALSE(Regex(quoted(LOREM_IPSUM)).match(LOREM_IPSUM.substr(1)));
    EXPECT_FALSE(Regex(quoted(LOREM_IPSUM)).match(LOREM_IPSUM.substr(0, LOREM_IPSUM.size() - 2)));
}

TEST(Regex, Range)
{
    EXPECT_EQ("[0-9]+"_rx.match("0123456789").view(), "0123456789"sv);
    EXPECT_EQ("[a-f]+"_rx.match("abcedef").view(), "abcedef"sv);
    EXPECT_EQ("[a-a]+"_rx.match("aaaaaaa").view(), "aaaaaaa"sv);
    EXPECT_EQ("[[-]]+"_rx.match("[\\]").view(), "[\\]"sv);
    EXPECT_EQ("[---]+"_rx.match("--").view(), "--"sv);

    EXPECT_FALSE("[a-z]"_rx.match("`"));
    EXPECT_FALSE("[a-z]"_rx.match("{"));

    EXPECT_THROW("["_rx, Error);
    EXPECT_THROW("[0"_rx, Error);
    EXPECT_THROW("[0-"_rx, Error);
    EXPECT_THROW("[0-9"_rx, Error);
    EXPECT_THROW("]"_rx, Error);
    EXPECT_THROW("9]"_rx, Error);
    EXPECT_THROW("-9]"_rx, Error);
    EXPECT_THROW("0-9]"_rx, Error);
}

TEST(Regex, Set)
{
    EXPECT_TRUE("a"_rx.match("a"));
    EXPECT_TRUE("o"_rx.match("+"));
    EXPECT_TRUE("n"_rx.match("7"));
    EXPECT_TRUE("Q"_rx.match("\""));
    EXPECT_TRUE("q"_rx.match("'"));

    EXPECT_FALSE("_"_rx.match("b"));
    EXPECT_FALSE("a"_rx.match("4"));
    EXPECT_FALSE("o"_rx.match("\t"));
    EXPECT_FALSE("n"_rx.match("|"));
    EXPECT_FALSE("Q"_rx.match("^"));
    EXPECT_FALSE("q"_rx.match("&"));
}

TEST(Regex, Sequence)
{
    EXPECT_TRUE("{'abc'}"_rx.match("abc"));
    EXPECT_TRUE("{'ab'} {'c'}"_rx.match("abc"));
    EXPECT_TRUE("{{{{{{'ab'} {'c'}}}}}}"_rx.match("abc"));

    EXPECT_THROW("{'abc'"_rx, Error);
    EXPECT_THROW("{"_rx, Error);
    EXPECT_THROW("}"_rx, Error);
    EXPECT_THROW("{{{'abc'"_rx, Error);
    EXPECT_THROW("'abc'}}}"_rx, Error);
}

TEST(Regex, Plus)
{
    EXPECT_TRUE("{'abc'}+"_rx.match("abcabcabc"));
    EXPECT_TRUE("{'ab'n}+"_rx.match("ab1ab2ab3"));
    EXPECT_TRUE("n+n+"_rx.match("12"));

    EXPECT_THROW("+"_rx, Error);
    EXPECT_THROW("++"_rx, Error);
    EXPECT_THROW("+a"_rx, Error);
    EXPECT_THROW("{}+"_rx, Error);
}

TEST(Regex, Star)
{
    EXPECT_TRUE("{'abc'}*"_rx.match("abc"));
    EXPECT_TRUE("{'abc'}*"_rx.match(""));
    EXPECT_TRUE("{'ab'n}*"_rx.match("ab1ab2ab3"));
    EXPECT_TRUE("{{{'hello'}}}*"_rx.match(""));
    EXPECT_TRUE("{{{'hello'}}}*"_rx.match("hellohellohello"));

    EXPECT_THROW("*"_rx, Error);
    EXPECT_THROW("***"_rx, Error);
    EXPECT_THROW("*a"_rx, Error);
    EXPECT_THROW("{}*"_rx, Error);
}

TEST(Regex, Quest)
{
    EXPECT_TRUE("{'abc'}?"_rx.match("abc"));
    EXPECT_TRUE("{'abc'}?"_rx.match(""));
    EXPECT_TRUE("{'ab'n}?"_rx.match("ab1"));
    EXPECT_TRUE("{{{'hello'}}}?"_rx.match(""));
    EXPECT_TRUE("{{{'hello'}}}?"_rx.match("hello"));

    EXPECT_THROW("?"_rx, Error);
    EXPECT_THROW("???"_rx, Error);
    EXPECT_THROW("?a"_rx, Error);
    EXPECT_THROW("{}?"_rx, Error);
}

TEST(Regex, Or)
{
    EXPECT_TRUE("{'a'|'b'}"_rx.match("a"));
    EXPECT_TRUE("{'a'|'b'}"_rx.match("a"));
    EXPECT_TRUE("{'a' | 'b'}"_rx.match("a"));
    EXPECT_TRUE("{'a' | 'b'}"_rx.match("b"));
    EXPECT_TRUE("a{a|'_'|n}*"_rx.match("snake_case_variable123"));

    EXPECT_THROW("|"_rx, Error);
    EXPECT_THROW("||"_rx, Error);
    EXPECT_THROW("|||"_rx, Error);
    EXPECT_THROW("'a'|{}"_rx, Error);
    EXPECT_THROW("{}|'b'"_rx, Error);
    EXPECT_THROW("'a'|"_rx, Error);
    EXPECT_THROW("|'b'"_rx, Error);
}

TEST(Regex, Wave)
{
    EXPECT_TRUE("^~'c'"_rx.match("abc"));
    EXPECT_TRUE("a~'z'"_rx.match("ahjklz"));
    EXPECT_EQ("'//' {a|' '} ~ '//'"_rx.match("// The program starts here // int main() {").view(),
              "// The program starts here //"sv);
    EXPECT_TRUE("n ~ {'z'|'9'}"_rx.match("0123456789"));
    EXPECT_TRUE("n ~ {'z'|'9'}"_rx.match("012345678z"));
    EXPECT_TRUE("{' '} ~ 'sus'"_rx.match("                           sus               "));
    EXPECT_FALSE("{' '} ~ 'sus'"_rx.match("            |             sus               "));

    EXPECT_THROW("~"_rx, Error);
    EXPECT_THROW("a~"_rx, Error);
    EXPECT_THROW("~{}"_rx, Error);
    EXPECT_THROW("{}~"_rx, Error);
}

TEST(Regex, Not)
{
    EXPECT_FALSE("'abc' !'d'"_rx.match("abcd"));
    EXPECT_EQ("'abc' !'d'"_rx.match("abc_").view(), "abc_");
    EXPECT_EQ("{!'\n'}*"_rx.match("lorem ipsum\n hello").view(), "lorem ipsum");
}

TEST(Regex, Dash)
{
    EXPECT_EQ("'abc'/'d'"_rx.match("abcd").view(), "abc");
    EXPECT_EQ("^~/_"_rx.match("words words").view(), "words");
}

} // namespace qcc::regex

#endif
