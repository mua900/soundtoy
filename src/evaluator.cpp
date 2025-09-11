#include "evaluator.h"

DArray<Token> tokenize(String expression)
{
    auto tokens = DArray<Token>(8);

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
    Expr* expr = parse_comparison_expr();
    if (expr)
    {
        collapse_expr(expr);
    }

    return expr;
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
        case TOKEN_TYPE_PAREN_OPEN:
        {
            cursor++;

            Expr* expr = parse_expression();

            if (!expr)
                return NULL;
            if (!consume(TOKEN_TYPE_PAREN_CLOSE))
                return NULL;

            return new Expr_Grouping(expr);
        }
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

void Evaluator::set(double sample_rate, double time)
{
    m_builtins[BUILTIN_TIME] = time;
    m_builtins[BUILTIN_SAMPLE_RATE] = sample_rate;
}

void Evaluator::update(double time)
{
    m_builtins[BUILTIN_TIME] = time;
}

void Evaluator::add(String expr_string)
{
    Parser parser;
    Array<Expr*> expressions = parser.parse(expr_string);
    for (int i = 0; i < expressions.size; i++)
    {
        Expr* expr = expressions.get(i);
        expr = collapse_expr(expr);
        m_expressions.add(expr);
    }
}

// simple depth first recursive tree traversal
Eval Evaluator::evaluate(Expr* expr)
{
    Eval fail = { 0.0, false };

    switch (expr->type)
    {
        case Expr_Type::Literal:
        {
            auto literal = static_cast<Expr_Literal*>(expr);
            if (literal->value.type == Value::VALUE_INTEGER)
            {
                // @todo integer math
                return { (double)literal->value.integer, true };
            }
            else if (literal->value.type == Value::VALUE_REAL)
            {
                return { literal->value.real, true };
            }
            else if (literal->value.type == Value::VALUE_STRING)
            {
                m_error.message = make_string("String literals aren't used and doesn't mean anything yet");
                return fail;
            }
            else
                panic("Unknown value type");  // @todo cleanup
        }
        case Expr_Type::Variable:
        {
            auto var = static_cast<Expr_Variable*>(expr);

            // builtin variables
            {
                if (string_compare(make_string("time"), var->name) ||
                    string_compare(make_string("t"), var->name)
                    )
                {
                    return { m_builtins[BUILTIN_TIME], true };
                }
                else if (string_compare(make_string("sample_rate"), var->name) ||
                    string_compare(make_string("sr"), var->name)
                    )
                {
                    return { m_builtins[BUILTIN_SAMPLE_RATE], true };
                }
            }

            m_error.message = make_string("Variable not defined"); // @todo error message
            return fail;
        }
        case Expr_Type::Unary:
        {
            auto unary = static_cast<Expr_Unary*>(expr);
            switch (unary->op)
            {
                case Unop_Negate:
                {
                    Eval eval = evaluate(unary->operand);
                    if (eval.success == false)
                        return fail;

                    eval.value = -eval.value;
                    return eval;
                }
                case Unop_Not:
                {
                    NOT_IMPLEMENTED("Logical operations")
                }
            }
        }
        case Expr_Type::Binary:
        {
            auto binary = static_cast<Expr_Binary*>(expr);
            Eval left = evaluate(binary->left);
            Eval right = evaluate(binary->right);

            if (left.success == false || right.success == false)
            {
                return fail;
            }

            switch (binary->op)
            {
                case Binop_Unknown: {
                    m_error.message = make_string("Invalid binary operator"); // @todo maybe this should be considered a bug case
                    return fail;
                }
                case Binop_Add: {
                    return { left.value + right.value, true };
                }
                case Binop_Sub: {
                    return { left.value - right.value, true };
                }
                case Binop_Mul: {
                    return { left.value * right.value, true };
                }
                case Binop_Div: {
                    return { left.value / right.value, true };
                }
                case Binop_Mod: {
                    return { fmod(left.value, right.value), true };  // @todo integer arithmetic
                }
                case Binop_Eq:
                case Binop_Neq:
                case Binop_Gt:
                case Binop_Ge:
                case Binop_Lt:
                case Binop_Le: {
                    NOT_IMPLEMENTED("Logical operations")
                }
            }
        }
        case Expr_Type::Grouping:
        {
            auto grouping = static_cast<Expr_Grouping*>(expr);
            return evaluate(grouping->expr);
        }
        case Expr_Type::Call:
        {
            NOT_IMPLEMENTED("Function calls")
        }
    }

    return fail;
}

void print_expr(Expr* expr, int indent)
{
    switch (expr->type)
    {
        case Expr_Type::Literal:
        {
            auto literal = static_cast<Expr_Literal*>(expr);
            switch (literal->value.type)
            {
                case Value::VALUE_INTEGER:
                    printf("Literal: %lli", literal->value.integer); break;
                case Value::VALUE_REAL:
                    printf("Literal: %f", literal->value.real); break;
                case Value::VALUE_STRING:
                    printf("Literal: %s", literal->value.string.data); break;
            }
            break;
        }
        case Expr_Type::Variable:
        {
            auto var = static_cast<Expr_Variable*>(expr);
            printf("Variable %s", var->name.data);
            break;
        }
        case Expr_Type::Unary:
        {
            auto unary = static_cast<Expr_Unary*>(expr);
            const char* operator_string = (unary->op == Unop_Negate) ? "Negate" : "Not";
            printf("Unary %s\n", operator_string);
            print_expr(unary->operand, indent + 1);
            break;
        }
        case Expr_Type::Binary:
        {
            auto binary = static_cast<Expr_Binary*>(expr);
            printf("Binary %s\n", get_binop_string(binary->op));
            print_expr(binary->left, indent + 1);
            print_expr(binary->right, indent + 1);
            break;
        }
        case Expr_Type::Grouping:
        {
            auto grouping = static_cast<Expr_Grouping*>(expr);
            printf("Grouping\n");
            print_expr(grouping->expr, indent + 1);
            break;
        }
        case Expr_Type::Call:
        {
            NOT_IMPLEMENTED("Function calls")
        }
    }
}

const char* get_binop_string(Op_Binary op)
{
    switch (op)
    {
        case Binop_Unknown: return "Binop_Unknown";
        case Binop_Add: return "Binop_Add";
        case Binop_Sub: return "Binop_Sub";
        case Binop_Mul: return "Binop_Mul";
        case Binop_Div: return "Binop_Div";
        case Binop_Mod: return "Binop_Mod";
        case Binop_Eq: return "Binop_Eq";
        case Binop_Neq: return "Binop_Neq";
        case Binop_Gt: return "Binop_Gt";
        case Binop_Ge: return "Binop_Ge";
        case Binop_Lt: return "Binop_Lt";
        case Binop_Le: return "Binop_Le";
    }

    return NULL;
}
