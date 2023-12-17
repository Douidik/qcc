#include "ast.hpp"
#include "parser.hpp"
#include "scan/scanner.hpp"
#include "scan/syntax_map.hpp"
#include "x86.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fmt::println(stderr, "no filepath provided in arguments\n");
        return 1;
    }

    std::fstream fstream(argv[1]);
    if (!fstream.is_open()) {
        fmt::println(stderr, "cannot open file from: '{}'\n", argv[1]);
        return 1;
    }

    std::string source = {std::istreambuf_iterator(fstream), {}};
    source.push_back('\n');

    qcc::Ast ast = {};
    qcc::Syntax_Map syntax_map = qcc::syntax_map_c89();
    qcc::Scanner scanner = {source, syntax_map};
    qcc::Parser(ast, scanner).parse();
    qcc::X86(ast, source, std::cout).make();
}
