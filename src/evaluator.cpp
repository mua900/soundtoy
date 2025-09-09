#include "evaluator.h"

DArray<Token> tokenize(String expression);

DArray<Token> tokenize(String expression)
{
    DArray<Token> tokens(8);

    int cursor = 0;
    while (cursor < expression.size)
    {
        char ch = expression.data[cursor];

        switch (ch)
        {
            case '+': tokens.add(Token(make_string("+"), TOKEN_TYPE_PLUS, cursor)); break;
            case '-': tokens.add(Token(make_string("-"), TOKEN_TYPE_MINUS, cursor)); break;
            case '*': tokens.add(Token(make_string("*"), TOKEN_TYPE_STAR, cursor)); break;
            case '/': tokens.add(Token(make_string("/"), TOKEN_TYPE_SLASH, cursor)); break;
            case '%': tokens.add(Token(make_string("%"), TOKEN_TYPE_PERCENT, cursor)); break;
            case ',': tokens.add(Token(make_string(","), TOKEN_TYPE_COMMA, cursor)); break;
            case ':': tokens.add(Token(make_string(":"), TOKEN_TYPE_COLON, cursor)); break;
            case ';': tokens.add(Token(make_string(";"), TOKEN_TYPE_SEMICOLON, cursor)); break;
            case '&': tokens.add(Token(make_string("&"), TOKEN_TYPE_AMPERSAND, cursor)); break;
            case '(': tokens.add(Token(make_string("("), TOKEN_TYPE_PAREN_OPEN, cursor)); break;
            case ')': tokens.add(Token(make_string(")"), TOKEN_TYPE_PAREN_CLOSE, cursor)); break;
            case '{': tokens.add(Token(make_string("{"), TOKEN_TYPE_BRACE_OPEN, cursor)); break;
            case '}': tokens.add(Token(make_string("}"), TOKEN_TYPE_BRACE_CLOSE, cursor)); break;
            case '!': {
                if (expression.data[cursor+1] == '=')
                {
                    tokens.add(Token(make_string("!="), TOKEN_TYPE_EXCLAMATION_EQUALS, cursor));
                }
                else
                {
                    tokens.add(Token(make_string("!"), TOKEN_TYPE_EXCLAMATION, cursor));
                }
                break;
            }
            case '=': {
                if (expression.data[cursor+1] == '=')
                {
                    tokens.add(Token(make_string("=="), TOKEN_TYPE_EQUALS_EQUALS, cursor));
                }
                else
                {
                    tokens.add(Token(make_string("="), TOKEN_TYPE_EQUALS, cursor));
                }
                break;
            }
            case '>': {
                if (expression.data[cursor+1] == '=')
                {
                    tokens.add(Token(make_string(">="), TOKEN_TYPE_GREATER_EQUALS, cursor));
                }
                else
                {
                    tokens.add(Token(make_string(">"), TOKEN_TYPE_GREATER, cursor));
                }
                break;
            }
            case '<': {
                if (expression.data[cursor+1] == '=')
                {
                    tokens.add(Token(make_string("<="), TOKEN_TYPE_LESS_EQUALS, cursor));
                }
                else
                {
                    tokens.add(Token(make_string("<"), TOKEN_TYPE_LESS, cursor));
                }
                break;
            }
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
                    tokens.add(Token(ident, TOKEN_TYPE_IDENT, cursor));
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
                        tokens.add(Token(ident, TOKEN_TYPE_LITERAL_FLOAT, cursor));
                    }
                    else
                    {
                        String ident = string_slice(expression, start, cursor);
                        tokens.add(Token(ident, TOKEN_TYPE_LITERAL_INT, cursor));
                    }
                }
            }
        }
    }

    tokens.add(Token(make_string("END"), TOKEN_TYPE_END, cursor));
    tokens.add(Token(make_string("END"), TOKEN_TYPE_END, cursor));

    return tokens;
}

Expr* collapse_expr(Expr* root)  // @todo implement
{
    Expr* expr = root;
    switch (expr->type)
    {
        case Expr_Type::Grouping:
        case Expr_Type::Binary:
        case Expr_Type::Call:
        case Expr_Type::Literal:
        case Expr_Type::Unary:
        {
            Expr_Unary* unary = static_cast<Expr_Unary*>(expr);
            
        }
        case Expr_Type::Variable:
            break;
    }

    return expr;
}

bool Parser::consume(Token_Type type)
{
    bool consumed = (tokens.get(cursor).type != type);
    if (consumed)
    {
        cursor++;
    }
    return consumed;
}

// @todo test and tie up with the application
Array<Expr*> Parser::parse(String expression_string)
{
    tokens = tokenize(expression_string);

    DArray<Expr*> expressions(8);

    while (true)
    {
        if (cursor < tokens.size)
        {
            LOG_ERROR("Malformed token stream");
            break;
        }

        if (tokens.get(cursor).type == TOKEN_TYPE_END)
        {
            break;
        }

        Expr* expr = parse_expression();
        if (!expr)
        {
            break;
        }

        expressions.add(expr);

        if (!consume(TOKEN_TYPE_SEMICOLON))
        {
            if (!consume(TOKEN_TYPE_END))
            {
                parser_error.message = make_string("Consecuent expressions should be seperated by ';'");
                parser_error.offset = tokens.get(cursor).offset;
            }

            break;
        }
    }

    return Array<Expr*>(expressions);
}

Expr* Parser::parse_expression()
{
    Expr* expr = parse_grouping_expr();
    if (expr)
    {
        collapse_expr(expr);
    }

    return expr;
}

Expr* Parser::parse_grouping_expr()
{
    if (tokens.get(cursor).type == TOKEN_TYPE_PAREN_OPEN)
    {
        cursor++;
        Expr* expr = parse_equality_expr();
        if (tokens.get(cursor).type == TOKEN_TYPE_PAREN_CLOSE)
        {
            cursor++;
            return new Expr_Grouping(expr);
        }
        else
            return NULL;
    }

    return parse_equality_expr();
}

Expr* Parser::parse_equality_expr()
{
    Expr* left = parse_comparison_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_PLUS || type == TOKEN_TYPE_MINUS)
    {
        cursor++;

        Expr* right = parse_comparison_expr();
        if (!right)
        {
            return NULL;
        }

        Op_Binary op = get_binop(type);
        if (op == Binop_Unknown) return NULL;  // this should be a bug if it happens

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_comparison_expr()
{
    Expr* left = parse_arithmetic_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_PLUS || type == TOKEN_TYPE_MINUS)
    {
        cursor++;

        Expr* right = parse_arithmetic_expr();
        if (!right)
        {
            return NULL;
        }

        Op_Binary op = get_binop(type);
        if (op == Binop_Unknown) return NULL;  // this should be a bug if it happens

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_arithmetic_expr()
{
    Expr* left = parse_factor_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_PLUS || type == TOKEN_TYPE_MINUS)
    {
        cursor++;

        Expr* right = parse_factor_expr();
        if (!right)
        {
            return NULL;
        }

        Op_Binary op = get_binop(type);
        if (op == Binop_Unknown) return NULL;  // this should be a bug if it happens

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_factor_expr()
{
    Expr* left = parse_unary_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_STAR || type == TOKEN_TYPE_SLASH)
    {
        cursor++;

        Expr* right = parse_unary_expr();
        if (!right)
        {
            return NULL;
        }

        Op_Binary op = get_binop(type);
        if (op == Binop_Unknown) return NULL;  // this should be a bug if it happens

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_unary_expr()
{
    Token_Type type = tokens.get(cursor).type;
    Expr* operand = NULL;
    while (type == TOKEN_TYPE_MINUS || type == TOKEN_TYPE_PLUS)
    {
        cursor++;

        operand = parse_expression();
        if (!operand)
        {
            return NULL;
        }

        Op_Unary op = (type == TOKEN_TYPE_MINUS) ? Unop_Negate : Unop_Not;
        operand = new Expr_Unary(op, operand);

        type = tokens.get(cursor).type;
    }

    if (operand)
    {
        return operand;
    }
    else {
        return parse_call_expr();
    }
}

Expr* Parser::parse_call_expr()
{
    Expr* primary = parse_primary_expr();
    if (!primary) return NULL;

    if (tokens.get(cursor).type != TOKEN_TYPE_PAREN_OPEN)
    {
        return primary;
    }

    if (!primary)
        return NULL;

    if (primary->type != Expr_Type::Variable)
    {
        return primary; //  what is this expression supposed to be when there is a parenthesis opening after it?
                        //  should we even return this if this is not defined and is going to fail when it tries to parse the next part?
    }

    Expr_Variable* var = static_cast<Expr_Variable*>(primary);

    Token_Type type = tokens.get(cursor).type;
    int arg_count = 0;
    int paren_close = 0;
    DArray<Expr*> arguments;

    // count and collect the arguments
    if (tokens.get(cursor).type != TOKEN_TYPE_PAREN_CLOSE)
    {
        arg_count++;

        while (true)
        {
            if (!(type != TOKEN_TYPE_END && cursor < tokens.size))
            {
                parser_error.message = make_string("Reached end of input before being able to parse all the arguments to function call"); // @todo bad error message
                parser_error.offset = paren_close;
                return NULL;
            }

            if (type == TOKEN_TYPE_PAREN_CLOSE)
            {
                break;
            }

            if (type == TOKEN_TYPE_COMMA)
                arg_count++;

            type = tokens.get(paren_close).type;
            paren_close++;
        }

        arguments = DArray<Expr*>(arg_count);

        while (cursor <= paren_close)
        {
            Expr* arg = parse_expression();
            if (!(arg && consume(TOKEN_TYPE_COMMA)))
            {
                arguments.free();
                return NULL;
            }

            arguments.add(arg);

            cursor++;
        }
    }

    return new Expr_Call(var->name, Array<Expr*>(arguments));
}

Expr* Parser::parse_primary_expr()
{
    Token token = tokens.get(cursor);
    Token_Type type = token.type;
    switch (type)
    {
        case TOKEN_TYPE_LITERAL_INT:
            cursor++;
            return new Expr_Literal(token.to_integer());
        case TOKEN_TYPE_LITERAL_FLOAT:
            cursor++;
            return new Expr_Literal(token.to_real());
        case TOKEN_TYPE_IDENT:
            cursor++;
            return new Expr_Variable(token.token_string);
        default:
            return NULL;
    }
}

Op_Binary get_binop(Token_Type type)
{
    switch (type)
    {
        case TOKEN_TYPE_PLUS:               return Binop_Add;
        case TOKEN_TYPE_MINUS:              return Binop_Sub;
        case TOKEN_TYPE_STAR:               return Binop_Mul;
        case TOKEN_TYPE_SLASH:              return Binop_Div;
        case TOKEN_TYPE_PERCENT:            return Binop_Mod;
        case TOKEN_TYPE_EQUALS_EQUALS:      return Binop_Eq;
        case TOKEN_TYPE_EXCLAMATION_EQUALS: return Binop_Neq;
        case TOKEN_TYPE_GREATER:            return Binop_Gt;
        case TOKEN_TYPE_GREATER_EQUALS:     return Binop_Ge;
        case TOKEN_TYPE_LESS:               return Binop_Lt;
        case TOKEN_TYPE_LESS_EQUALS:        return Binop_Le;
        default:
            return Binop_Unknown;
    }
}
