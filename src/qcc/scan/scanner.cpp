#include "scanner.hpp"

namespace qcc
{

Scanner::Scanner(std::string_view src, Syntax_Map &map) : source(src), next(src), map(map) {}

Token Scanner::tokenize(int128 skip_mask)
{
    Token token = {};

    do {
        if (next.empty()) {
            return dummy_token(Token_Eof);
        }

        for (const auto &[type, regex] : map) {
            if (auto match = regex.match(next)) {
                next = match.next();
                token.str = match.view();
                token.type = type;
                token.type_str = token_type_str(token.type);

                if (!token.type) {
                    throw errorf("unrecognized token", token);
                }
                break;
            }
        }
    } while (token.type & skip_mask);

    return token;
}

Token Scanner::dummy_token(Token_Type type) const
{
    return Token{std::string_view{&source.back(), 1}, type, true};
}

} // namespace qcc
