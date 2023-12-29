#ifndef QCC_X86_TEST_HPP
#define QCC_X86_TEST_HPP

#include "allocator.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "scan/scanner.hpp"
#include "scan/syntax_map.hpp"
#include "x86.hpp"
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sys/wait.h>

#ifndef QCC_TEST_PATH
  #define QCC_TEST_PATH "test/"
#endif

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
        exit(execlp("/bin/sh", "sh", "-c", cmd.c_str(), NULL));
    }
}

std::string make_build_filepath(std::string_view filename, std::string_view extension)
{
    fs::path directory = fs::temp_directory_path();
    return fmt::format("{}/{}.{}", directory.string(), filename, extension);
}

static testing::AssertionResult expect_return(std::string_view file, int expected_return)
{
    fs::path directory = fs::temp_directory_path();
    fs::path filepath = file;
    std::string filename = filepath.stem().string();
    std::string filepath_asm = make_build_filepath(filename, "s");
    std::string filepath_obj = make_build_filepath(filename, "o");
    std::string filepath_exe = make_build_filepath(filename, "");

    std::fstream fstream(filepath);
    if (!fstream.is_open()) {
        return testing::AssertionFailure() << "cannot open file from: " << filepath;
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

    std::fstream fstream_asm(filepath_asm, std::ios::out | std::ios::trunc);
    X86 x86 = {ast, allocator, source, fstream_asm};
    x86.make();
    fstream_asm.close();

    if (exec_cmd("nasm -g -f elf64 {}", filepath_asm) != 0) {
        return testing::AssertionFailure() << filepath << ": nasm error";
    }
    if (exec_cmd("ld -m elf_x86_64 {} -o {}", filepath_obj, filepath_exe) != 0) {
        return testing::AssertionFailure() << filepath << ": ld error";
    }

    int exe_return = exec_cmd("{}", filepath_exe);
    if (exe_return != expected_return) {
        return testing::AssertionFailure()
               << filepath << ": exited with: " << exe_return << ", expected: " << expected_return;
    }
    return testing::AssertionSuccess();
}

#define Expect_Return(filepath, expected_return) \
    EXPECT_TRUE(expect_return(QCC_TEST_PATH filepath, expected_return))

TEST(X86, Common)
{
    Expect_Return("add.c", 55 + 3);
    Expect_Return("pointer.c", 132);
    Expect_Return("use_ranges.c", 5);
    Expect_Return("struct.c", 102);
}

} // namespace qcc

#endif
