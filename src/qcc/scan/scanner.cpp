#include "scanner.hpp"

namespace qcc
{

const fs::path Libs = "/usr/lib";

Scanner::Scanner(std::string_view file, fs::path cwd) : file{file, file}, cwd(cwd) {}

Token Scanner::tokenize(Syntax_Map syntax_map, int128 skip_mask)
{
    Token token = {};

    do {
        auto &[data, stream] = current_source();

        if (stream.empty()) {
            return dummy_token(Token_Eof);
        }
        for (const auto &[type, regex] : syntax_map) {
            if (Regex_Match match = regex.match(stream)) {
                token.str = match.view();
                token.type = type;
                token.source = data;
                token.type_str = token_type_str(token.type);
                stream = match.next();

                if (!token.type) {
                    throw errorf("unrecognized token", token);
                }
                break;
            }
        }
    } while (token.type & skip_mask);

    if (token.type & Token_Id and definition_map.contains(token.str)) {
        Scanner_Source definition = definition_map.at(token.str);
        macro_stack.push(definition);
        return tokenize(syntax_map, skip_mask);
    }
    if (token.type & Token_Mask_Hash) {
        return parse_hash_token(token);
    }

    return token;
}

Scanner_Source &Scanner::current_source()
{
    if (!macro_stack.empty()) {
        Scanner_Source &macro_stream = macro_stack.top();
        if (!macro_stream.stream.empty())
            return macro_stream;
        return macro_stack.pop(), current_source();
    }

    return file;
}

Token Scanner::dummy_token(Token_Type type) const
{
    return Token{std::string_view{&file.data.back(), 1}, type, true};
}

Token Scanner::parse_hash_token(Token token)
{
    switch (token.type) {
    case Token_Hash_Include: {
        Token include_token = tokenize(syntax_map_include());
        std::fstream fstream;

        if (include_token.type & (Token_Hash_System_Filepath | Token_Hash_Project_Filepath)) {
            fs::path filepath = include_token.str.substr(1, include_token.str.size() - 2);

            if (include_token.type & Token_Hash_System_Filepath) {
                fstream.open(Libs / filepath, std::ios::in);
            }
            if (include_token.type & Token_Hash_Project_Filepath or !fstream.is_open()) {
                fstream.open(cwd / filepath, std::ios::in);
            }

            if (!fstream.is_open()) {
                throw errorf("file not found: '{}'", token, filepath.string());
            }
            std::string include_source = fstream_to_str(std::move(fstream));
            includes.emplace_back(std::move(include_source));
            std::string_view include = includes.back();
            macro_stack.push({include, include});
        }

        break;
    }

    default:
        qcc_assert("Todo! parse_hash_token() support for provided hash directive", 0);
    }

    return tokenize(syntax_map_c89());
}

} // namespace qcc
