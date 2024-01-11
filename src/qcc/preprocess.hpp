#ifndef QCC_PREPROCESS_HPP
#define QCC_PREPROCESS_HPP

#include "scan/scanner.hpp"
#include <unordered_map>

namespace qcc
{

struct Preprocessor
{
    Scanner scanner;
    std::string filepath;
    fs::path cwd;
    std::vector<Token> tokens;
    std::vector<Token> macros;
    std::vector<std::string> files;
    std::unordered_map<std::string_view, Source_Context> sources;

    Preprocessor(fs::path filepath);

    void process();
    void process_context(Source_Context *context, bool has_eof = false, int128 skip_mask = Token_Mask_Skip);
    void process_hash_token(Source_Context *context, Token hash);
    Source_Context fs_open(fs::path filepath, Token token);

    Error errorf(std::string_view fmt, Token token, auto... args) const
    {
        return Error{"preprocess error", make_source_snippet(token, fmt, args...)};
    }
};

} // namespace qcc

#endif
