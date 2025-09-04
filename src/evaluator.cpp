#include "evaluator.h"

enum Token_Type {
    TOKEN_TYPE_UNKNOWN = 0,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_STAR,
    TOKEN_TYPE_SLASH,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_AMPERSAND,
    TOKEN_TYPE_PAREN_OPEN,
    TOKEN_TYPE_PAREN_CLOSE,
    TOKEN_TYPE_BRACE_OPEN,
    TOKEN_TYPE_BRACE_CLOSE,

    TOKEN_TYPE_IDENT,
    TOKEN_TYPE_LITERAL_INT,
    TOKEN_TYPE_LITERAL_FLOAT,
    // TT_LITERAL_STRING,
};

struct Token {
    Token_Type type = TOKEN_TYPE_UNKNOWN;
    String token_string;

    Token() {}
    Token(String s, Token_Type t_type) : type(t_type), token_string(s) {}
};

DArray<Token> tokenize(String expression);

Array<Expr*> parse_expression(String expression)
{
    DArray<Token> tokens = tokenize(expression);
    // @todo parse the expression

    return Array<Expr*>();
}

DArray<Token> tokenize(String expression)
{
    DArray<Token> tokens(8);

    int cursor = 0;
    while (cursor < expression.size)
    {
        char ch = expression.data[cursor];

        switch (ch)
        {
            case '+': tokens.add(Token(make_string("+"), TOKEN_TYPE_PLUS)); break;
            case '-': tokens.add(Token(make_string("-"), TOKEN_TYPE_MINUS)); break;
            case '*': tokens.add(Token(make_string("*"), TOKEN_TYPE_STAR)); break;
            case '/': tokens.add(Token(make_string("/"), TOKEN_TYPE_SLASH)); break;
            case ',': tokens.add(Token(make_string(","), TOKEN_TYPE_COMMA)); break;
            case ':': tokens.add(Token(make_string(":"), TOKEN_TYPE_COLON)); break;
            case ';': tokens.add(Token(make_string(";"), TOKEN_TYPE_SEMICOLON)); break;
            case '&': tokens.add(Token(make_string("&"), TOKEN_TYPE_AMPERSAND)); break;
            case '(': tokens.add(Token(make_string("("), TOKEN_TYPE_PAREN_OPEN)); break;
            case ')': tokens.add(Token(make_string(")"), TOKEN_TYPE_PAREN_CLOSE)); break;
            case '{': tokens.add(Token(make_string("{"), TOKEN_TYPE_BRACE_OPEN)); break;
            case '}': tokens.add(Token(make_string("}"), TOKEN_TYPE_BRACE_CLOSE)); break;
            default:
            {

                if (is_space(ch))
                {
                    while (is_space(ch))
                    {
                        ch = expression.data[cursor];
                        cursor++;
                    }
                    continue;
                }

                const auto is_ident_character = [](char ch){return is_alpha(ch) || ch == '_';};
                if (is_ident_character(ch))
                {
                    int start = cursor;
                    while (is_ident_character(ch))
                    {
                        cursor++;
                        ch = expression.data[cursor];
                    }

                    String ident = string_slice(expression, start, cursor);
                    tokens.add(Token(ident, TOKEN_TYPE_IDENT));
                }
                else if (is_digit(ch))
                {
                    int start = cursor;
                    while (is_digit(ch)) { cursor++; ch = expression.data[cursor]; }

                    if (ch == '.')
                    {
                        cursor++;
                        ch = expression.data[cursor];
                        while (is_digit(ch)) { cursor++; ch = expression.data[cursor]; }
                        String ident = string_slice(expression, start, cursor);
                        tokens.add(Token(ident, TOKEN_TYPE_LITERAL_FLOAT));
                    }
                    else
                    {
                        String ident = string_slice(expression, start, cursor);
                        tokens.add(Token(ident, TOKEN_TYPE_LITERAL_INT));
                    }
                }
            }
        }
    }

    return tokens;
}
