#include "allocator.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "scan/scanner.hpp"
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
    fs::path directory = fs::temp_directory_path();
    std::string filename = filepath.stem().string();
    std::string filepath_asm = make_build_filepath(filename, ".s");
    std::string filepath_obj = make_build_filepath(filename, ".o");
    if (output == "?") {
        output = make_build_filepath(filename, "");
    }

    Ast ast = {};

    std::fstream source_fstream(filepath);
    if (!source_fstream.is_open()) {
        fmt::println(stderr, "cannot open source file from '{}'", filepath.string());
        return 1;	
    }

    std::string source = fstream_to_str(std::move(source_fstream));
    Scanner scanner = {source, filepath.parent_path()};
    Parser parser = {ast, scanner};
    parser.parse();
    Allocator allocator = {ast, 7, 7};
    allocator.allocate();
    if (verbose) {
        ast.dump_statement(std::cout, (Statement *)ast.main_statement, 0);
    }
    std::fstream fstream_asm(filepath_asm, std::ios::out | std::ios::trunc);
    X86 x86 = {ast, allocator, source, fstream_asm};
    x86.emit();
    fstream_asm.close();

    if (exec_cmd("nasm -g -f elf64 {}", filepath_asm) != 0) {
        fmt::println(stderr, "nasm command failed");
        return 1;
    }
    if (exec_cmd("ld -m elf_x86_64 {} -o {}", filepath_obj, output.string()) != 0) {
        fmt::println(stderr, "ld command failed");
        return 1;
    }
    return 0;
}

} // namespace qcc

int main(int argc, char *argv[])
{
    bool verbose = false;
    std::string_view filepath = "";
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
            filepath = argv[optind];
            break;
        }
    }

    return qcc::x86_compile(filepath, output, verbose);
}
