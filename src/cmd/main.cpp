#include "allocator.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "scan/scanner.hpp"
#include "scan/syntax_map.hpp"
#include "x86.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/wait.h>

namespace qcc
{

int x86_run(std::filesystem::path filepath)
{
    std::fstream fstream(filepath);
    if (!fstream.is_open()) {
        fmt::println(stderr, "cannot open file from: '{}'\n", filepath.string());
        return 1;
    }
    std::string source = {std::istreambuf_iterator(fstream), {}};
    source.push_back('\n');

    Ast ast = {};
    Syntax_Map syntax_map = syntax_map_c89();
    Scanner scanner = {source, syntax_map};

    Parser parser = {ast, scanner};
    parser.parse();
    Allocator allocator = {ast, 7, 7};
    allocator.allocate();
    // ast.dump_statement(std::cout, (Statement *)ast.main_statement, 0);
    
    X86 x86 = {ast, allocator, source, std::cout};
    x86.make();

    return 0;
}

} // namespace qcc

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fmt::println(stderr, "no filepath provided in arguments\n");
        return 1;
    }
    return qcc::x86_run(argv[1]);
}
