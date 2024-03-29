#include "allocator.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "preprocess.hpp"
#include "x86.hpp"
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sys/wait.h>

namespace qcc
{

int exec_cmd(std::string_view fmt, auto... args)
{
    std::string cmd = fmt::format(fmt::runtime(fmt), args...);
    pid_t pid = fork();

    if (pid != 0) {
        int wait_status;
        wait(&wait_status);
        return WEXITSTATUS(wait_status);
    } else {
        int32 cmd_status = execlp("/bin/sh", "sh", "-c", cmd.c_str(), NULL);
        exit(cmd_status);
    }
}

std::string make_build_filepath(std::string_view filename, std::string_view extension)
{
    fs::path directory = fs::temp_directory_path();
    return fmt::format("{}/{}{}", directory.string(), filename, extension);
}

int x86_compile(fs::path filepath, fs::path output, bool verbose)
{
    fs::path directory = filepath.parent_path();
    std::string filename = filepath.stem().string();
    std::string filepath_asm = make_build_filepath(filename, ".s");
    std::string filepath_obj = make_build_filepath(filename, ".o");

    if (output == "?") {
        output = directory / filename;
    }

    std::fstream source_fstream(filepath);
    if (!source_fstream.is_open()) {
        fmt::println(stderr, "cannot open source file from '{}'", filepath.string());
        return 1;
    }

    Ast ast = {};
    Preprocessor preprocessor = {filepath.string()};
    preprocessor.process();
    if (verbose) {
        int32 pad = 0;
        for (Token &token : preprocessor.tokens) {
            pad = Max(pad, token.str.size());
        }
        for (Token &token : preprocessor.tokens) {
            fmt::println(stderr, "{:{}?}{}", token.str, pad + 4, token.type_str);
        }
    }

    Parser parser = {ast, &preprocessor.tokens[0], verbose};
    parser.parse();
    Allocator allocator = {ast, 7, 7};
    allocator.allocate();
    if (verbose) {
        ast.dump_statement(std::cerr, (Statement *)ast.main_statement, 0);
    }

    std::fstream fstream_asm(filepath_asm, std::ios::out | std::ios::trunc);
    X86 x86 = {ast, allocator, fstream_asm};
    x86.emit();
    fstream_asm.close();

    if (exec_cmd("nasm -g -f elf64 {}", filepath_asm) != 0) {
        fmt::println(stderr, "nasm command failed with:");
        fmt::println(stdout, "{}", fstream_to_str(std::fstream{filepath_asm}));
        return 1;
    }
    if (exec_cmd("ld -m elf_x86_64 {} -o {}", filepath_obj, output.string()) != 0) {
        fmt::println(stderr, "ld command failed with:");
        fmt::println(stdout, "{}", fstream_to_str(std::fstream{filepath_obj}));
        return 1;
    }
    return 0;
}

} // namespace qcc

const std::string_view Usage = //
    "./qcc -f <source-filepath> -o <output> -v\n"
    " -f: C source code filepath\n"
    " -o: output path, defaulted to (dir/filename.c => dir/filename)\n"
    " -v: verbose mode, prints the ast";

int main(int argc, char *argv[])
{
    bool verbose = false;
    std::string_view filepath = "?";
    std::string_view output = "?";

    for (int opt; (opt = getopt(argc, argv, "o:f:v")) != -1;) {
        switch (opt) {
        case 'v':
            verbose = true;
            break;
        case 'f':
            filepath = optarg;
            break;
        case 'o':
            output = optarg;
            break;

        default:
            fmt::println(stderr, "unknown command line option '{}'", opt);
            fmt::println(stderr, "{}", Usage);
            return 1;
        }
    }

    if (filepath == "?") {
        fmt::println(stderr, "no source filepath provided");
        fmt::println(stderr, "{}", Usage);
        return 1;
    }

    return qcc::x86_compile(filepath, output, verbose);
}
