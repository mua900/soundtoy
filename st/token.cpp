#include "token.h"

void print_token(Token& token, String_Builder* sb, bool ignore_some)
{
    sb->clear();

    if (ignore_some)
    {
        if (token.type == TOKEN_TYPE_END ||
            token.type == TOKEN_TYPE_UNKNOWN ||
            token.type == TOKEN_TYPE_SEMICOLON
            )
        {
            return;
        }
    }

    const char* type_name = get_token_type_string(token.type);
    sb->append(make_string(type_name));
    sb->append_char(' ');
    sb->append(token.token_string);
    printf("%s %d\n", sb->c_string(), token.offset);
}

const char* get_token_type_string(Token_Type type)
{
    switch (type)
    {
        case TOKEN_TYPE_UNKNOWN:            return "TOKEN_TYPE_UNKNOWN";
        case TOKEN_TYPE_PLUS:               return "TOKEN_TYPE_PLUS";
        case TOKEN_TYPE_MINUS:              return "TOKEN_TYPE_MINUS";
        case TOKEN_TYPE_STAR:               return "TOKEN_TYPE_STAR";
        case TOKEN_TYPE_SLASH:              return "TOKEN_TYPE_SLASH";
        case TOKEN_TYPE_PERCENT:            return "TOKEN_TYPE_PERCENT";
        case TOKEN_TYPE_COMMA:              return "TOKEN_TYPE_COMMA";
        case TOKEN_TYPE_COLON:              return "TOKEN_TYPE_COLON";
        case TOKEN_TYPE_SEMICOLON:          return "TOKEN_TYPE_SEMICOLON";
        case TOKEN_TYPE_AMPERSAND:          return "TOKEN_TYPE_AMPERSAND";
        case TOKEN_TYPE_PAREN_OPEN:         return "TOKEN_TYPE_PAREN_OPEN";
        case TOKEN_TYPE_PAREN_CLOSE:        return "TOKEN_TYPE_PAREN_CLOSE";
        case TOKEN_TYPE_BRACE_OPEN:         return "TOKEN_TYPE_BRACE_OPEN";
        case TOKEN_TYPE_BRACE_CLOSE:        return "TOKEN_TYPE_BRACE_CLOSE";
     	case TOKEN_TYPE_QUESTION_MARK:      return "TOKEN_TYPE_QUESTION_MARK";
        case TOKEN_TYPE_EXCLAMATION:        return "TOKEN_TYPE_EXCLAMATION";
        case TOKEN_TYPE_EQUALS:             return "TOKEN_TYPE_EQUALS";
        case TOKEN_TYPE_GREATER:            return "TOKEN_TYPE_GREATER";
        case TOKEN_TYPE_LESS:               return "TOKEN_TYPE_LESS";
        case TOKEN_TYPE_EXCLAMATION_EQUALS: return "TOKEN_TYPE_EXCLAMATION_EQUALS";
        case TOKEN_TYPE_EQUALS_EQUALS:      return "TOKEN_TYPE_EQUALS_EQUALS";
        case TOKEN_TYPE_GREATER_EQUALS:     return "TOKEN_TYPE_GREATER_EQUALS";
        case TOKEN_TYPE_LESS_EQUALS:        return "TOKEN_TYPE_LESS_EQUALS";
        case TOKEN_TYPE_IDENT:              return "TOKEN_TYPE_IDENT";
        case TOKEN_TYPE_LITERAL_INT:        return "TOKEN_TYPE_LITERAL_INT";
        case TOKEN_TYPE_LITERAL_FLOAT:      return "TOKEN_TYPE_LITERAL_FLOAT";
        // TT_LITERAL_STRING
        case TOKEN_TYPE_END:                return "TOKEN_TYPE_END";
    }
}
