#include "scanner.hpp"

namespace qcc
{

Scanner::Scanner(std::string_view src, Syntax_Map &map) : src(src), next(src), map(map) {}

Token Scanner::tokenize()
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
                break;
            }
        }
    } while (token.type & (Token_Blank | Token_Comment));

    return token;
}

Token Scanner::dummy_token(Token_Type type) const
{
    return Token{std::string_view{&src.back(), 1}, type, true};
}

} // namespace qcc
