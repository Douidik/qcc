#ifndef QCC_SCAN_TEST_HPP
#define QCC_SCAN_TEST_HPP

#include "scan/scanner.hpp"
#include <gtest/gtest.h>

namespace qcc
{

TEST(Scan, Syntax_Map_Test)
{
    EXPECT_NO_THROW(syntax_map_c89());
}

static testing::AssertionResult match_tokens(std::string_view source_view, std::vector<Token> tokens)
{
    std::string source(source_view);
    source.push_back('\n');

    Scanner scanner = {source, ""};
    for (Token expected : tokens) {
        Token token = scanner.tokenize(syntax_map_c89(), Token_Blank);

        if (token.type != expected.type) {
            std::string_view token_type = token_type_str(token.type);
            std::string_view expected_type = token_type_str(expected.type);
            return testing::AssertionFailure() << "'" << token_type << "' != '" << expected_type << "'";
        }
        if (token.str != expected.str) {
            return testing::AssertionFailure() << "'" << token.str << "' != '" << expected.str << "'";
        }
    }
    return testing::AssertionSuccess();
}

static void scan_source(std::string_view source_view)
{
    std::string source(source_view);
    source.push_back('\n');
    Scanner scanner = {source, ""};
    for (Token token; token.type != Token_Eof;) {
        token = scanner.tokenize(syntax_map_c89(), Token_Blank);
    }
}

#define Expect_Tokens(source, ...) EXPECT_TRUE(match_tokens(source, {__VA_ARGS__}))
#define Expect_Scanner_Error(source) EXPECT_THROW(scan_source(source), Error)

TEST(Scanner, Comment)
{
    Expect_Tokens("// Hello World \n", {"// Hello World ", Token_Comment});

    Expect_Tokens(
        R"(
// This is a comment 
int main() {
// A second one //
)",
        {"// This is a comment ", Token_Comment}, {"int", Token_Int_Type}, {"main", Token_Id},
        {"(", Token_Paren_Begin}, {")", Token_Paren_End}, {"{", Token_Scope_Begin},
        {"// A second one //", Token_Comment});

    Expect_Tokens("/* Hello World */\n", {"/* Hello World */", Token_Comment});

    Expect_Tokens(
        R"(
/*
 * main() -> int
 * Arguments: None
 * Return: Program execution status, if main() != 0, then the program execution failed
 * This function is the program starting point of the program, must be declared in any program
*/
int main() {
/* This is an inline comment, that is delimited by :*/
)",

        {R"(/*
 * main() -> int
 * Arguments: None
 * Return: Program execution status, if main() != 0, then the program execution failed
 * This function is the program starting point of the program, must be declared in any program
*/)",
         Token_Comment},
        {"int", Token_Int_Type}, {"main", Token_Id}, {"(", Token_Paren_Begin}, {")", Token_Paren_End},
        {"{", Token_Scope_Begin}, {"/* This is an inline comment, that is delimited by :*/", Token_Comment});

    Expect_Tokens(
        R"(
// Mutiline single-line comments\
didn't know it was a thing \
   now i know        \

auto main() -> Integer { return EXIT_SUCCESS; }
)",
        {
            R"(// Mutiline single-line comments\
didn't know it was a thing \
   now i know        \
)",
            Token_Comment});

#if 0
    Expect_Tokens(
        R"(
    // This multiline comment is malformed \<Should be spaces !!!>
    ninja spaces are preventing me to be a comment, just a bunch of identifiers
    )",
        {R"(// This multiline comment is malformed \ )", Token_Comment},
        {"ninja", Token_Id},
        {"spaces", Token_Id},
        {"are", Token_Id},
        {"preventing", Token_Id},
        {"me", Token_Id},
        {"to", Token_Id},
        {"be", Token_Id},
        {"a", Token_Id},
        {"comment", Token_Id},
        {",", Token_Comma},
        {"just", Token_Id},
        {"a", Token_Id},
        {"bunch", Token_Id},
        {"of", Token_Id},
        {"identifiers", Token_Id});
#endif
}

#if 0
TEST(Lexer, Directive)
{
    Expect_Tokens("#define TRUE (1)\n", {"#define TRUE (1)", Token_Directive});
    Expect_Tokens("#define FALSE (0)\n", {"#define FALSE (0)", Token_Directive});

    Expect_Tokens(
        R"(
#define POW(x, n) do { for (int i = 0; i < n; i++) { \
  x = x * x;                                         \
} while (0)

int main() {}
)",
        {
            R"(#define POW(x, n) do { for (int i = 0; i < n; i++) { \
  x = x * x;                                         \
} while (0))",
            Token_Directive,
        });

    Expect_Tokens("#  include <stdio.h>\n", {"#  include <stdio.h>", Token_Directive});

    Expect_Tokens(
        R"(
#ifndef CCC_EXAMPLE_H
#  define CCC_EXAMPLE_H

#  define CCC_VAL \
  (-1)  

#endif
)",
        {"#ifndef CCC_EXAMPLE_H", Token_Directive}, {"#  define CCC_EXAMPLE_H", Token_Directive},
        {"#  define CCC_VAL \\\n  (-1)  ", Token_Directive}, {"#endif", Token_Directive});
}
#endif

TEST(Lexer, Keyword)
{
    Expect_Tokens("auto", {"auto", Token_Auto});
    Expect_Tokens("double", {"double", Token_Double_Type});
    Expect_Tokens("char", {"char", Token_Char_Type});
    Expect_Tokens("float", {"float", Token_Float_Type});
    Expect_Tokens("int", {"int", Token_Int_Type});
    Expect_Tokens("void", {"void", Token_Void_Type});
    Expect_Tokens("long", {"long", Token_Long});
    Expect_Tokens("short", {"short", Token_Short});
    Expect_Tokens("enum", {"enum", Token_Enum});
    Expect_Tokens("typedef", {"typedef", Token_Typedef});
    Expect_Tokens("union", {"union", Token_Union});
    Expect_Tokens("struct", {"struct", Token_Struct});
    Expect_Tokens("volatile", {"volatile", Token_Volatile});
    Expect_Tokens("const", {"const", Token_Const});
    Expect_Tokens("extern", {"extern", Token_Extern});
    Expect_Tokens("register", {"register", Token_Register});
    Expect_Tokens("static", {"static", Token_Static});
    Expect_Tokens("signed", {"signed", Token_Signed});
    Expect_Tokens("unsigned", {"unsigned", Token_Unsigned});
    Expect_Tokens("*", {"*", Token_Pointer});
    Expect_Tokens("break", {"break", Token_Break});
    Expect_Tokens("case", {"case", Token_Case});
    Expect_Tokens("continue", {"continue", Token_Continue});
    Expect_Tokens("default", {"default", Token_Default});
    Expect_Tokens("do", {"do", Token_Do});
    Expect_Tokens("else", {"else", Token_Else});
    Expect_Tokens("for", {"for", Token_For});
    Expect_Tokens("goto", {"goto", Token_Goto});
    Expect_Tokens("if", {"if", Token_If});
    Expect_Tokens("return", {"return", Token_Return});
    Expect_Tokens("switch", {"switch", Token_Switch});
    Expect_Tokens("while", {"while", Token_While});
    Expect_Tokens("sizeof", {"sizeof", Token_Sizeof});

    Expect_Tokens("        signed", {"signed", Token_Signed});
    Expect_Tokens("signed        ", {"signed", Token_Signed});
}

TEST(Lexer, Operator)
{
    Expect_Tokens("++", {"++", Token_Increment});
    Expect_Tokens("--", {"--", Token_Decrement});
    Expect_Tokens("{", {"{", Token_Scope_Begin});
    Expect_Tokens("}", {"}", Token_Scope_End});
    Expect_Tokens("(", {"(", Token_Paren_Begin});
    Expect_Tokens(")", {")", Token_Paren_End});
    Expect_Tokens("[", {"[", Token_Crochet_Begin});
    Expect_Tokens("]", {"]", Token_Crochet_End});
    Expect_Tokens(":", {":", Token_Colon});
    Expect_Tokens(";", {";", Token_Semicolon});
    Expect_Tokens(",", {",", Token_Comma});
    Expect_Tokens("&&", {"&&", Token_And});
    Expect_Tokens("||", {"||", Token_Or});
    Expect_Tokens(".", {".", Token_Dot});
    Expect_Tokens("->", {"->", Token_Arrow});
    Expect_Tokens("~", {"~", Token_Bitwise_Not});
    Expect_Tokens("|", {"|", Token_Bitwise_Or});
    Expect_Tokens("^", {"^", Token_Bitwise_Xor});
    Expect_Tokens("<<", {"<<", Token_Shift_L});
    Expect_Tokens(">>", {">>", Token_Shift_R});
    Expect_Tokens("+", {"+", Token_Add});
    Expect_Tokens("-", {"-", Token_Sub});
    Expect_Tokens("/", {"/", Token_Div});
    Expect_Tokens("%", {"%", Token_Mod});
    Expect_Tokens("==", {"==", Token_Eq});
    Expect_Tokens("!=", {"!=", Token_Not_Eq});
    Expect_Tokens("<=", {"<=", Token_Less_Eq});
    Expect_Tokens(">=", {">=", Token_Greater_Eq});
    Expect_Tokens("<", {"<", Token_Less});
    Expect_Tokens(">", {">", Token_Greater});
    Expect_Tokens("!", {"!", Token_Not});
    Expect_Tokens("=", {"=", Token_Assign});
    Expect_Tokens("&", {"&", Token_Ampersand});

    // Operators doesn't require spaces to match
    Expect_Tokens("!finished", {"!", Token_Not}, {"finished", Token_Id});
    Expect_Tokens("! finished", {"!", Token_Not}, {"finished", Token_Id});

    // (Add/Sub) to (Increment/Decrement) Ambiguity
    Expect_Tokens("i++", {"i", Token_Id}, {"++", Token_Increment});
    Expect_Tokens("++i", {"++", Token_Increment}, {"i", Token_Id});
    Expect_Tokens("i ++", {"i", Token_Id}, {"++", Token_Increment});
    Expect_Tokens("++ i", {"++", Token_Increment}, {"i", Token_Id});
    Expect_Tokens("+ +", {"+", Token_Add}, {"+", Token_Add});

    Expect_Tokens("vec.x", {"vec", Token_Id}, {".", Token_Dot}, {"x", Token_Id});
    Expect_Tokens("pvec->size", {"pvec", Token_Id}, {"->", Token_Arrow}, {"size", Token_Id});
}

TEST(Lexer, Integer)
{
    Expect_Tokens("1234567890", {"1234567890", Token_Int});
    Expect_Tokens("0", {"0", Token_Int});
    Expect_Tokens("-0", {"-", Token_Sub}, {"0", Token_Int});
    Expect_Tokens("+0", {"+", Token_Add}, {"0", Token_Int});
    Expect_Tokens("+2147483647", {"+", Token_Add}, {"2147483647", Token_Int});
    Expect_Tokens("-2147483648", {"-", Token_Sub}, {"2147483648", Token_Int});
    Expect_Tokens("-+0", {"-", Token_Sub}, {"+", Token_Add}, {"0", Token_Int});
    Expect_Tokens("+-+0", {"+", Token_Add}, {"-", Token_Sub}, {"+", Token_Add}, {"0", Token_Int});
    Expect_Tokens("++0", {"++", Token_Increment}, {"0", Token_Int});

    // A prefix without a becomes a suffix :-|
    Expect_Tokens("0b", {"0b", Token_Int});
    Expect_Tokens("0x", {"0x", Token_Int});

    Expect_Tokens("0b0", {"0b0", Token_Int_Bin});
    Expect_Tokens("0B1", {"0B1", Token_Int_Bin});
    Expect_Tokens("0b1011101", {"0b1011101", Token_Int_Bin});
    Expect_Tokens("0B1011101", {"0B1011101", Token_Int_Bin});

    Expect_Tokens("0x0", {"0x0", Token_Int_Hex});
    Expect_Tokens("0X0", {"0X0", Token_Int_Hex});
    Expect_Tokens("0xF", {"0xF", Token_Int_Hex});
    Expect_Tokens("0XF", {"0XF", Token_Int_Hex});
    Expect_Tokens("0xf", {"0xf", Token_Int_Hex});
    Expect_Tokens("0xF", {"0xF", Token_Int_Hex});
    Expect_Tokens("0xaBcDeF0123456789", {"0xaBcDeF0123456789", Token_Int_Hex});
    Expect_Tokens("+0xaBcDeF0123456789", {"+", Token_Add}, {"0xaBcDeF0123456789", Token_Int_Hex});
    Expect_Tokens("-0xaBcDeF0123456789", {"-", Token_Sub}, {"0xaBcDeF0123456789", Token_Int_Hex});
    Expect_Tokens("-+0xAbCdEf0123456789", {"-", Token_Sub}, {"+", Token_Add},
                  {"0xAbCdEf0123456789", Token_Int_Hex});
    Expect_Tokens("+-0XaBcdEf0123456789", {"+", Token_Add}, {"-", Token_Sub},
                  {"0XaBcdEf0123456789", Token_Int_Hex});

    Expect_Tokens("1234567890e0123456789", {"1234567890e0123456789", Token_Int});
    Expect_Tokens("1234567890e+0123456789", {"1234567890e+0123456789", Token_Int});
    Expect_Tokens("1234567890e-0123456789", {"1234567890e-0123456789", Token_Int});

    // Matches the 'e' but not a valid character suffix
    Expect_Tokens("1234567890e-+0123456789", {"1234567890e", Token_Int});
    Expect_Tokens("0b10e0123456789", {"0b10e", Token_Int_Bin});

    // Matches the 'e' as an hexdecimal digit
    Expect_Tokens("0xdeadbeefe0123456789", {"0xdeadbeefe0123456789", Token_Int_Hex});
    Expect_Tokens("0xdeadbeefe+0123456789", {"0xdeadbeefe", Token_Int_Hex}, {"+", Token_Add},
                  {"0123456789", Token_Int});
    Expect_Tokens("0xdeadbeefe-0123456789", {"0xdeadbeefe", Token_Int_Hex}, {"-", Token_Sub},
                  {"0123456789", Token_Int});
}

/*
{"-", Token_Sub}, {"+", Add},
*/

TEST(Lexer, Float)
{
    Expect_Tokens("0.0", {"0.0", Token_Float});
    Expect_Tokens(".0", {".0", Token_Float});
    Expect_Tokens("0.", {"0.", Token_Float});

    Expect_Tokens("1234567890.0123456789", {"1234567890.0123456789", Token_Float});
    Expect_Tokens(".0123456789", {".0123456789", Token_Float});
    Expect_Tokens("1234567890.", {"1234567890.", Token_Float});

    Expect_Tokens("+1234567890.0123456789", {"+", Token_Add}, {"1234567890.0123456789", Token_Float});
    Expect_Tokens("+.0123456789", {"+", Token_Add}, {".0123456789", Token_Float});
    Expect_Tokens("+1234567890.", {"+", Token_Add}, {"1234567890.", Token_Float});

    Expect_Tokens("-1234567890.0123456789", {"-", Token_Sub}, {"1234567890.0123456789", Token_Float});
    Expect_Tokens("-.0123456789", {"-", Token_Sub}, {".0123456789", Token_Float});
    Expect_Tokens("-1234567890.", {"-", Token_Sub}, {"1234567890.", Token_Float});

    Expect_Tokens("+-1234567890.0123456789", {"+", Token_Add}, {"-", Token_Sub},
                  {"1234567890.0123456789", Token_Float});
    Expect_Tokens("-+.0123456789", {"-", Token_Sub}, {"+", Token_Add}, {".0123456789", Token_Float});
    Expect_Tokens("+-+1234567890.", {"+", Token_Add}, {"-", Token_Sub}, {"+", Token_Add},
                  {"1234567890.", Token_Float});

    Expect_Tokens("1234567890.0123456789e1234567890", {"1234567890.0123456789e1234567890", Token_Float});
    Expect_Tokens("1234567890.0123456789e+1234567890", {"1234567890.0123456789e+1234567890", Token_Float});
    Expect_Tokens("1234567890.0123456789e-1234567890", {"1234567890.0123456789e-1234567890", Token_Float});
    Expect_Tokens(".0123456789e+1234567890", {".0123456789e+1234567890", Token_Float});
    Expect_Tokens("1234567890.e+1234567890", {"1234567890.e+1234567890", Token_Float});
    Expect_Tokens(".0123456789e-1234567890", {".0123456789e-1234567890", Token_Float});
    Expect_Tokens("1234567890.e-1234567890", {"1234567890.e-1234567890", Token_Float});

    Expect_Tokens("1234567890.0123456789e", {"1234567890.0123456789e", Token_Float});
    Expect_Tokens("1234567890.0123456789e+-", {"1234567890.0123456789e", Token_Float});
    Expect_Tokens("1234567890.0123456789e-+", {"1234567890.0123456789e", Token_Float});
    Expect_Tokens("1234567890.0123456789e--", {"1234567890.0123456789e", Token_Float},
                  {"--", Token_Decrement});
    Expect_Tokens("1234567890.0123456789e++", {"1234567890.0123456789e", Token_Float},
                  {"++", Token_Increment});
}

TEST(Lexer, String)
{
    Expect_Tokens(R"("" // empty string)", {R"("")", Token_String});
    Expect_Tokens(R"(L"" // empty string)", {R"(L"")", Token_String});

    Expect_Tokens(R"("Character sequence constant literal")",
                  {R"("Character sequence constant literal")", Token_String});

    Expect_Tokens(R"(L"Character sequence constant literal")",
                  {R"(L"Character sequence constant literal")", Token_String});

    Expect_Tokens(R"( "\"" )", {R"("\"")", Token_String});
    Expect_Tokens(R"("\"Hello\" world")", {R"("\"Hello\" world")", Token_String});

    Expect_Tokens(
        R"("My string\
is immunised \
to")",
        {R"("My string\
is immunised \
to")",
         Token_String});

    Expect_Scanner_Error("\"");
    Expect_Scanner_Error("\"\"\"");

    Expect_Scanner_Error("\"");
    Expect_Scanner_Error("\"Weak string\n\"");
    Expect_Scanner_Error("\"\n\"");
}

TEST(Lexer, Char)
{
    Expect_Tokens("'c'", {"'c'", Token_Char});
    Expect_Tokens("L'c'", {"L'c'", Token_Char});

    // Escape sequences
    Expect_Tokens(R"('\'')", {R"('\'')", Token_Char});
    Expect_Tokens(R"('\"')", {R"('\"')", Token_Char});
    Expect_Tokens(R"('\?')", {R"('\?')", Token_Char});
    Expect_Tokens(R"('\\')", {R"('\\')", Token_Char});
    Expect_Tokens(R"('\a')", {R"('\a')", Token_Char});
    Expect_Tokens(R"('\b')", {R"('\b')", Token_Char});
    Expect_Tokens(R"('\f')", {R"('\f')", Token_Char});
    Expect_Tokens(R"('\n')", {R"('\n')", Token_Char});
    Expect_Tokens(R"('\r')", {R"('\r')", Token_Char});
    Expect_Tokens(R"('\t')", {R"('\t')", Token_Char});
    Expect_Tokens(R"('\v')", {R"('\v')", Token_Char});

#if 0
    // Wide Character constants 
    Expect_Tokens("'abc'", {"'abc'", Token_Char});
    Expect_Tokens("'32f'", {"'32f'", Token_Char});
#endif

    Expect_Scanner_Error("'");
    Expect_Scanner_Error("''");
    Expect_Scanner_Error("'''");
}

TEST(Lexer, LongSource)
{
    Expect_Tokens(
        R"(
/* return non-zero if magic sequence is detected */
/* warning: reset the read pointer to the beginning of the file */
int detect_magic(FILE* f) {
  unsigned char buffer[8];
  size_t bytes_read;
  int c;

  fseek(f, SEEK_SET, 0);
  bytes_read = fread(buffer, 1, 8, f);
  fseek(f, SEEK_SET, 0);
  if (bytes_read < 8) return 0;

  for (c = 0; c < 8; c++)
    if (buffer[c] != sixpack_magic[c]) return 0;

  return -1;
}

static unsigned long readU16(const unsigned char* ptr) { return ptr[0] + (ptr[1] << 8); }

static unsigned long readU32(const unsigned char* ptr) {
  return ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
}

void read_chunk_header(FILE* f, int* id, int* options, unsigned long* size, unsigned long* checksum,
                       unsigned long* extra) {
  unsigned char buffer[16];
  fread(buffer, 1, 16, f);

  *id = readU16(buffer) & 0xffff;
  *options = readU16(buffer + 2) & 0xffff;
  *size = readU32(buffer + 4) & 0xffffffff;
  *checksum = readU32(buffer + 8) & 0xffffffff;
  *extra = readU32(buffer + 12) & 0xffffffff;
}

int unpack_file(const char* input_file) {
  FILE* in;
  unsigned long fsize;
  int c;
  unsigned long percent;
  unsigned char progress[20];
  int chunk_id;
  int chunk_options;
  unsigned long chunk_size;
  unsigned long chunk_checksum;
  unsigned long chunk_extra;
  unsigned char buffer[BLOCK_SIZE];
  unsigned long checksum;

  unsigned long decompressed_size;
  unsigned long total_extracted;
  int name_length;
  char* output_file;
  FILE* f;

  unsigned char* compressed_buffer;
  unsigned char* decompressed_buffer;
  unsigned long compressed_bufsize;
  unsigned long decompressed_bufsize;

  /* sanity check */
  in = fopen(input_file, "rb");
  if (!in) {
    printf("Error: could not open %s\n", input_file);
    return -1;
  }
)",
        {"/* return non-zero if magic sequence is detected */", Token_Comment},
        {"/* warning: reset the read pointer to the beginning of the file */", Token_Comment},
        {"int", Token_Int_Type}, {"detect_magic", Token_Id}, {"(", Token_Paren_Begin}, {"FILE", Token_Id},
        {"*", Token_Star}, {"f", Token_Id}, {")", Token_Paren_End}, {"{", Token_Scope_Begin},
        {"unsigned", Token_Unsigned}, {"char", Token_Char_Type}, {"buffer", Token_Id},
        {"[", Token_Crochet_Begin}, {"8", Token_Int}, {"]", Token_Crochet_End}, {";", Token_Semicolon},
        {"size_t", Token_Id}, {"bytes_read", Token_Id}, {";", Token_Semicolon}, {"int", Token_Int_Type},
        {"c", Token_Id}, {";", Token_Semicolon}, {"fseek", Token_Id}, {"(", Token_Paren_Begin},
        {"f", Token_Id}, {",", Token_Comma}, {"SEEK_SET", Token_Id}, {",", Token_Comma}, {"0", Token_Int},
        {")", Token_Paren_End}, {";", Token_Semicolon}, {"bytes_read", Token_Id}, {"=", Token_Assign},
        {"fread", Token_Id}, {"(", Token_Paren_Begin}, {"buffer", Token_Id}, {",", Token_Comma},
        {"1", Token_Int}, {",", Token_Comma}, {"8", Token_Int}, {",", Token_Comma}, {"f", Token_Id},
        {")", Token_Paren_End}, {";", Token_Semicolon}, {"fseek", Token_Id}, {"(", Token_Paren_Begin},
        {"f", Token_Id}, {",", Token_Comma}, {"SEEK_SET", Token_Id}, {",", Token_Comma}, {"0", Token_Int},
        {")", Token_Paren_End}, {";", Token_Semicolon}, {"if", Token_If}, {"(", Token_Paren_Begin},
        {"bytes_read", Token_Id}, {"<", Token_Less}, {"8", Token_Int}, {")", Token_Paren_End},
        {"return", Token_Return}, {"0", Token_Int}, {";", Token_Semicolon}, {"for", Token_For},
        {"(", Token_Paren_Begin}, {"c", Token_Id}, {"=", Token_Assign}, {"0", Token_Int},
        {";", Token_Semicolon}, {"c", Token_Id}, {"<", Token_Less}, {"8", Token_Int}, {";", Token_Semicolon},
        {"c", Token_Id}, {"++", Token_Increment}, {")", Token_Paren_End}, {"if", Token_If},
        {"(", Token_Paren_Begin}, {"buffer", Token_Id}, {"[", Token_Crochet_Begin}, {"c", Token_Id},
        {"]", Token_Crochet_End}, {"!=", Token_Not_Eq}, {"sixpack_magic", Token_Id},
        {"[", Token_Crochet_Begin}, {"c", Token_Id}, {"]", Token_Crochet_End}, {")", Token_Paren_End},
        {"return", Token_Return}, {"0", Token_Int}, {";", Token_Semicolon}, {"return", Token_Return},
        {"-", Token_Sub}, {"1", Token_Int}, {";", Token_Semicolon}, {"}", Token_Scope_End},
        {"static", Token_Static}, {"unsigned", Token_Unsigned}, {"long", Token_Long}, {"readU16", Token_Id},
        {"(", Token_Paren_Begin}, {"const", Token_Const}, {"unsigned", Token_Unsigned},
        {"char", Token_Char_Type}, {"*", Token_Star}, {"ptr", Token_Id}, {")", Token_Paren_End},
        {"{", Token_Scope_Begin}, {"return", Token_Return}, {"ptr", Token_Id}, {"[", Token_Crochet_Begin},
        {"0", Token_Int}, {"]", Token_Crochet_End}, {"+", Token_Add}, {"(", Token_Paren_Begin},
        {"ptr", Token_Id}, {"[", Token_Crochet_Begin}, {"1", Token_Int}, {"]", Token_Crochet_End},
        {"<<", Token_Shift_L}, {"8", Token_Int}, {")", Token_Paren_End}, {";", Token_Semicolon},
        {"}", Token_Scope_End}, {"static", Token_Static}, {"unsigned", Token_Unsigned}, {"long", Token_Long},
        {"readU32", Token_Id}, {"(", Token_Paren_Begin}, {"const", Token_Const}, {"unsigned", Token_Unsigned},
        {"char", Token_Char_Type}, {"*", Token_Star}, {"ptr", Token_Id}, {")", Token_Paren_End},
        {"{", Token_Scope_Begin}, {"return", Token_Return}, {"ptr", Token_Id}, {"[", Token_Crochet_Begin},
        {"0", Token_Int}, {"]", Token_Crochet_End}, {"+", Token_Add}, {"(", Token_Paren_Begin},
        {"ptr", Token_Id}, {"[", Token_Crochet_Begin}, {"1", Token_Int}, {"]", Token_Crochet_End},
        {"<<", Token_Shift_L}, {"8", Token_Int}, {")", Token_Paren_End}, {"+", Token_Add},
        {"(", Token_Paren_Begin}, {"ptr", Token_Id}, {"[", Token_Crochet_Begin}, {"2", Token_Int},
        {"]", Token_Crochet_End}, {"<<", Token_Shift_L}, {"16", Token_Int}, {")", Token_Paren_End},
        {"+", Token_Add}, {"(", Token_Paren_Begin}, {"ptr", Token_Id}, {"[", Token_Crochet_Begin},
        {"3", Token_Int}, {"]", Token_Crochet_End}, {"<<", Token_Shift_L}, {"24", Token_Int},
        {")", Token_Paren_End}, {";", Token_Semicolon}, {"}", Token_Scope_End}, {"void", Token_Void_Type},
        {"read_chunk_header", Token_Id}, {"(", Token_Paren_Begin}, {"FILE", Token_Id}, {"*", Token_Star},
        {"f", Token_Id}, {",", Token_Comma}, {"int", Token_Int_Type}, {"*", Token_Star}, {"id", Token_Id},
        {",", Token_Comma}, {"int", Token_Int_Type}, {"*", Token_Star}, {"options", Token_Id},
        {",", Token_Comma}, {"unsigned", Token_Unsigned}, {"long", Token_Long}, {"*", Token_Star},
        {"size", Token_Id}, {",", Token_Comma}, {"unsigned", Token_Unsigned}, {"long", Token_Long},
        {"*", Token_Star}, {"checksum", Token_Id}, {",", Token_Comma}, {"unsigned", Token_Unsigned},
        {"long", Token_Long}, {"*", Token_Star}, {"extra", Token_Id}, {")", Token_Paren_End},
        {"{", Token_Scope_Begin}, {"unsigned", Token_Unsigned}, {"char", Token_Char_Type},
        {"buffer", Token_Id}, {"[", Token_Crochet_Begin}, {"16", Token_Int}, {"]", Token_Crochet_End},
        {";", Token_Semicolon}, {"fread", Token_Id}, {"(", Token_Paren_Begin}, {"buffer", Token_Id},
        {",", Token_Comma}, {"1", Token_Int}, {",", Token_Comma}, {"16", Token_Int}, {",", Token_Comma},
        {"f", Token_Id}, {")", Token_Paren_End}, {";", Token_Semicolon}, {"*", Token_Star}, {"id", Token_Id},
        {"=", Token_Assign}, {"readU16", Token_Id}, {"(", Token_Paren_Begin}, {"buffer", Token_Id},
        {")", Token_Paren_End}, {"&", Token_Ampersand}, {"0xffff", Token_Int_Hex}, {";", Token_Semicolon},
        {"*", Token_Star}, {"options", Token_Id}, {"=", Token_Assign}, {"readU16", Token_Id},
        {"(", Token_Paren_Begin}, {"buffer", Token_Id}, {"+", Token_Add}, {"2", Token_Int},
        {")", Token_Paren_End}, {"&", Token_Ampersand}, {"0xffff", Token_Int_Hex}, {";", Token_Semicolon},
        {"*", Token_Star}, {"size", Token_Id}, {"=", Token_Assign}, {"readU32", Token_Id},
        {"(", Token_Paren_Begin}, {"buffer", Token_Id}, {"+", Token_Add}, {"4", Token_Int},
        {")", Token_Paren_End}, {"&", Token_Ampersand}, {"0xffffffff", Token_Int_Hex}, {";", Token_Semicolon},
        {"*", Token_Star}, {"checksum", Token_Id}, {"=", Token_Assign}, {"readU32", Token_Id},
        {"(", Token_Paren_Begin}, {"buffer", Token_Id}, {"+", Token_Add}, {"8", Token_Int},
        {")", Token_Paren_End}, {"&", Token_Ampersand}, {"0xffffffff", Token_Int_Hex}, {";", Token_Semicolon},
        {"*", Token_Star}, {"extra", Token_Id}, {"=", Token_Assign}, {"readU32", Token_Id},
        {"(", Token_Paren_Begin}, {"buffer", Token_Id}, {"+", Token_Add}, {"12", Token_Int},
        {")", Token_Paren_End}, {"&", Token_Ampersand}, {"0xffffffff", Token_Int_Hex}, {";", Token_Semicolon},
        {"}", Token_Scope_End}, {"int", Token_Int_Type}, {"unpack_file", Token_Id}, {"(", Token_Paren_Begin},
        {"const", Token_Const}, {"char", Token_Char_Type}, {"*", Token_Star}, {"input_file", Token_Id},
        {")", Token_Paren_End}, {"{", Token_Scope_Begin}, {"FILE", Token_Id}, {"*", Token_Star},
        {"in", Token_Id}, {";", Token_Semicolon}, {"unsigned", Token_Unsigned}, {"long", Token_Long},
        {"fsize", Token_Id}, {";", Token_Semicolon}, {"int", Token_Int_Type}, {"c", Token_Id},
        {";", Token_Semicolon}, {"unsigned", Token_Unsigned}, {"long", Token_Long}, {"percent", Token_Id},
        {";", Token_Semicolon}, {"unsigned", Token_Unsigned}, {"char", Token_Char_Type},
        {"progress", Token_Id}, {"[", Token_Crochet_Begin}, {"20", Token_Int}, {"]", Token_Crochet_End},
        {";", Token_Semicolon}, {"int", Token_Int_Type}, {"chunk_id", Token_Id}, {";", Token_Semicolon},
        {"int", Token_Int_Type}, {"chunk_options", Token_Id}, {";", Token_Semicolon},
        {"unsigned", Token_Unsigned}, {"long", Token_Long}, {"chunk_size", Token_Id}, {";", Token_Semicolon},
        {"unsigned", Token_Unsigned}, {"long", Token_Long}, {"chunk_checksum", Token_Id},
        {";", Token_Semicolon}, {"unsigned", Token_Unsigned}, {"long", Token_Long}, {"chunk_extra", Token_Id},
        {";", Token_Semicolon}, {"unsigned", Token_Unsigned}, {"char", Token_Char_Type}, {"buffer", Token_Id},
        {"[", Token_Crochet_Begin}, {"BLOCK_SIZE", Token_Id}, {"]", Token_Crochet_End},
        {";", Token_Semicolon}, {"unsigned", Token_Unsigned}, {"long", Token_Long}, {"checksum", Token_Id},
        {";", Token_Semicolon}, {"unsigned", Token_Unsigned}, {"long", Token_Long},
        {"decompressed_size", Token_Id}, {";", Token_Semicolon}, {"unsigned", Token_Unsigned},
        {"long", Token_Long}, {"total_extracted", Token_Id}, {";", Token_Semicolon}, {"int", Token_Int_Type},
        {"name_length", Token_Id}, {";", Token_Semicolon}, {"char", Token_Char_Type}, {"*", Token_Star},
        {"output_file", Token_Id}, {";", Token_Semicolon}, {"FILE", Token_Id}, {"*", Token_Star},
        {"f", Token_Id}, {";", Token_Semicolon}, {"unsigned", Token_Unsigned}, {"char", Token_Char_Type},
        {"*", Token_Star}, {"compressed_buffer", Token_Id}, {";", Token_Semicolon},
        {"unsigned", Token_Unsigned}, {"char", Token_Char_Type}, {"*", Token_Star},
        {"decompressed_buffer", Token_Id}, {";", Token_Semicolon}, {"unsigned", Token_Unsigned},
        {"long", Token_Long}, {"compressed_bufsize", Token_Id}, {";", Token_Semicolon},
        {"unsigned", Token_Unsigned}, {"long", Token_Long}, {"decompressed_bufsize", Token_Id},
        {";", Token_Semicolon}, {"/* sanity check */", Token_Comment}, {"in", Token_Id}, {"=", Token_Assign},
        {"fopen", Token_Id}, {"(", Token_Paren_Begin}, {"input_file", Token_Id}, {",", Token_Comma},
        {"\"rb\"", Token_String}, {")", Token_Paren_End}, {";", Token_Semicolon}, {"if", Token_If},
        {"(", Token_Paren_Begin}, {"!", Token_Not}, {"in", Token_Id}, {")", Token_Paren_End},
        {"{", Token_Scope_Begin}, {"printf", Token_Id}, {"(", Token_Paren_Begin},
        {"\"Error: could not open %s\\n\"", Token_String}, {",", Token_Comma}, {"input_file", Token_Id},
        {")", Token_Paren_End}, {";", Token_Semicolon}, {"return", Token_Return}, {"-", Token_Sub},
        {"1", Token_Int}, {";", Token_Semicolon}, {"}", Token_Scope_End});
}

} // namespace qcc

#endif
