#include "scanner.hpp"

namespace qcc
{

Token Scanner::tokenize(Source_Context *context, Syntax_Map syntax_map, int128 skip_mask)
{
    Token token = {};

    if (context->stream.empty()) {
        token.str = std::string_view{&context->stream.back(), 1};
        token.type = Token_Eof;
        token.ok = true;
        token.context = *context;
	return token;
    }

    do {
        for (const auto &[type, regex] : syntax_map) {
            if (Regex_Match match = regex.match(context->stream)) {
                token.str = match.view();
                token.type = type;
                token.context = *context;
                token.type_str = token_type_str(token.type);
                context->stream = match.next();

                if (!token.type) {
                    throw errorf("unrecognized token", token);
                }
                break;
            }
        }
    } while (token.type & skip_mask);

    return token;
}

} // namespace qcc
