#ifndef QCC_X86_TEST_HPP
#define QCC_X86_TEST_HPP

#include "allocator.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "preprocess.hpp"
#include "x86.hpp"
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sys/wait.h>

#ifndef Qcc_Test_Path
#define Qcc_Test_Path "test/"
#endif

namespace qcc
{

static int exec_cmd(std::string_view fmt, auto... args)
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

static std::string make_build_filepath(std::string_view filename, std::string_view extension)
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

    Ast ast = {};
    Preprocessor preprocessor = {filepath.string()};
    preprocessor.process();
    Parser parser = {ast, &preprocessor.tokens[0], false};
    parser.parse();
    Allocator allocator = {ast, 7, 7};
    allocator.allocate();

    std::fstream fstream_asm(filepath_asm, std::ios::out | std::ios::trunc);
    X86 x86 = {ast, allocator, fstream_asm};
    x86.emit();
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
    EXPECT_TRUE(expect_return(Qcc_Test_Path filepath, expected_return))
// We choose 1 as the success exit code because it's easier to deal with equality operators
#define Expect_Ok(filepath) EXPECT_TRUE(expect_return(Qcc_Test_Path filepath, 1))

TEST(X86, Common)
{
    Expect_Ok("add.c");
    Expect_Ok("mul.c");
    Expect_Ok("pointer.c");
    Expect_Ok("use_ranges.c");
    Expect_Ok("struct.c");
    Expect_Ok("header.c");
    Expect_Ok("precedence.c");
    Expect_Ok("binary_assignment.c");
    Expect_Ok("array.c");
}

} // namespace qcc

#endif
