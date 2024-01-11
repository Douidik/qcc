#include "preprocess.hpp"

namespace qcc
{

const fs::path Libs = "/lib/";

Preprocessor::Preprocessor(fs::path filepath) :
    filepath(fs::absolute(filepath).string()), cwd(filepath.parent_path())
{
}

void Preprocessor::process()
{
    Token &filepath_token = macros.emplace_back(Token{filepath});
    Source_Context main_context = fs_open(filepath, filepath_token);
    process_context(&main_context, true);
}

void Preprocessor::process_context(Source_Context *context, bool has_eof, int128 skip_mask)
{
    Token token = {};
    while (token.type != Token_Eof) {
        token = scanner.tokenize(context, syntax_map_c89(), skip_mask);

        if (token.type & Token_Newline)
            context->line++;
        else if (token.type & Token_Mask_Hash)
            process_hash_token(context, token);
        else if (has_eof or token.type != Token_Eof) {
            tokens.push_back(token);
	}
    }
}

void Preprocessor::process_hash_token(Source_Context *context, Token hash)
{
    switch (hash.type) {
    case Token_Hash_Include: {
        Token filepath_token = scanner.tokenize(context, syntax_map_include());
        fs::path filepath = {
            filepath_token.str.begin() + 1,
            filepath_token.str.end() - 1,
        };

        if (filepath_token.type & Token_Hash_System_Filepath)
            filepath = Libs / filepath;
        if (filepath_token.type & Token_Hash_Cwd_Filepath or !fs::exists(filepath))
            filepath = cwd / filepath;

        Source_Context include_context = fs_open(filepath, filepath_token);
        process_context(&include_context);
        break;
    }
    default:
        qcc_assert(0, "Todo! process_hash_token() support for hash directive type");
    }
}

Source_Context Preprocessor::fs_open(fs::path filepath, Token token)
{
    if (!fs::exists(filepath)) {
        throw errorf("file does not exists: {}", token, filepath.string());
    }

    std::string file = fstream_to_str(std::fstream(filepath));
    std::string_view source = files.emplace_back(std::move(file));

    Source_Context context = {};
    context.source = source;
    context.stream = source;
    context.filepath = &macros.emplace_back(token);
    context.line = 0;
    context.hash = (int64)&context;

    return context;
}

} // namespace qcc
