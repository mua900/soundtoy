#include "evaluator.h"
#include <math.h>

DArray<Token> tokenize(String expression)
{
    auto tokens = DArray<Token>(8);

#define ADD_TOKEN(p_token_type, p_token_string) tokens.add(Token(make_string(p_token_string), p_token_type, cursor)); cursor++;

    int cursor = 0;
    while (cursor < expression.size)
    {
        char ch = expression.data[cursor];

        switch (ch)
        {
            case '+':
                ADD_TOKEN(TOKEN_TYPE_PLUS, "+")
                break;
            case '-':
                ADD_TOKEN(TOKEN_TYPE_MINUS, "-")
                break;
            case '*':
                ADD_TOKEN(TOKEN_TYPE_STAR, "*")
                break;
            case '/':
                ADD_TOKEN(TOKEN_TYPE_SLASH, "/")
                break;
            case '%':
                ADD_TOKEN(TOKEN_TYPE_PERCENT, "%")
                break;
            case ',':
                ADD_TOKEN(TOKEN_TYPE_COMMA, ",")
                break;
            case ':':
                ADD_TOKEN(TOKEN_TYPE_COLON, ":")
                break;
            case ';':
                ADD_TOKEN(TOKEN_TYPE_SEMICOLON, ";")
                break;
            case '&':
                ADD_TOKEN(TOKEN_TYPE_AMPERSAND, "&")
                break;
            case '(':
                ADD_TOKEN(TOKEN_TYPE_PAREN_OPEN, "(")
                break;
            case ')':
                ADD_TOKEN(TOKEN_TYPE_PAREN_CLOSE, ")")
                break;
            case '{':
                ADD_TOKEN(TOKEN_TYPE_BRACE_OPEN, "{")
                break;
            case '}':
                ADD_TOKEN(TOKEN_TYPE_BRACE_CLOSE, "}")
                break;
            case '!': {
                if (expression.data[cursor+1] == '=')
                {
                    ADD_TOKEN(TOKEN_TYPE_EXCLAMATION_EQUALS, "!=")
                }
                else
                {
                    ADD_TOKEN(TOKEN_TYPE_EXCLAMATION, "!")
                }
                break;
            }
            case '=': {
                if (expression.data[cursor+1] == '=')
                {
                    ADD_TOKEN(TOKEN_TYPE_EQUALS_EQUALS, "==")
                }
                else
                {
                    ADD_TOKEN(TOKEN_TYPE_EQUALS, "=")
                }
                break;
            }
            case '>': {
                if (expression.data[cursor+1] == '=')
                {
                    ADD_TOKEN(TOKEN_TYPE_GREATER_EQUALS, ">=");
                }
                else
                {
                    ADD_TOKEN(TOKEN_TYPE_GREATER, ">")
                }
                break;
            }
            case '<': {
                if (expression.data[cursor+1] == '=')
                {
                    ADD_TOKEN(TOKEN_TYPE_LESS_EQUALS, "<=");
                }
                else
                {
                    ADD_TOKEN(TOKEN_TYPE_LESS, "<");
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
                else {
                    LOG_MSGF("Unknown character %c", ch);
                    cursor++;
                }
            }
        }
    }

    tokens.add(Token(make_string("END"), TOKEN_TYPE_END, cursor));
    tokens.add(Token(make_string("END"), TOKEN_TYPE_END, cursor));

    return tokens;
}

bool Parser::consume(Token_Type type)
{
    bool match = (tokens.get(cursor).type == type);
    if (match)
    {
        cursor++;
    }
    return match;
}

Array<Expr*> Parser::parse(String expression_string)
{
    tokens = tokenize(expression_string);

    DArray<Expr*> expressions(8);

    while (true)
    {
        if (cursor >= tokens.size)
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
    Expr* expr = parse_equality_expr();
    if (expr)
    {
        String error_string = {};
        expr = collapse_expr(expr, &error_string);

        if (!expr)
        {
            printf("Collapse expression failed\n");
        }
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

    // function call
    cursor++;  // (

    if (primary->type != Expr_Type::Variable)
    {
        return primary; //  what is this expression supposed to be when there is a parenthesis opening after it?
                        //  should we even return this if this is not defined and is going to fail when it tries to parse the next part?
    }

    Expr_Variable* var = static_cast<Expr_Variable*>(primary);

    int arg_count = 0;
    int paren_close = 0;
    DArray<Expr*> arguments;

    // count and collect the arguments
    if (tokens.get(cursor).type != TOKEN_TYPE_PAREN_CLOSE)
    {
        arg_count++;

        while (true)
        {
            if (!(tokens.get(paren_close).type != TOKEN_TYPE_END && cursor < tokens.size))
            {
                parser_error.message = make_string("Reached end of input before being able to parse all the arguments to function call"); // @todo bad error message
                parser_error.offset = paren_close;
                return NULL;
            }

            if (tokens.get(paren_close).type == TOKEN_TYPE_PAREN_CLOSE)
            {
                break;
            }

            if (tokens.get(paren_close).type == TOKEN_TYPE_COMMA)
                arg_count++;

            paren_close++;
        }
    }

    arguments = DArray<Expr*>(arg_count);

    while (cursor < paren_close)
    {
        Expr* arg = parse_expression();

        if (arg)
            arguments.add(arg);

        if (cursor == paren_close) break;

        if (!(arg && tokens.get(cursor).type == TOKEN_TYPE_COMMA))
        {
            arguments.free();
            return NULL;
        }

        cursor++;
    }

    cursor++; // )

    Function_ID fn_id = get_function_id(var->name);
    if (fn_id == FUNC_ID_INVALID)
    {
        return NULL;
    }

    return new Expr_Call(var->name, Array<Expr*>(arguments), fn_id);
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

const char* Builtin_Func_Names[BUILTIN_FUNC_COUNT] = {
    "BUILTIN_FUNC_SIN",
    "BUILTIN_FUNC_COS",
    "BUILTIN_FUNC_ABS",
    "BUILTIN_FUNC_SIGN",
    "BUILTIN_FUNC_ARCSIN",
    "BUILTIN_FUNC_ARCCOS",
};


void Evaluator::set(double sample_rate, double time)
{
    m_builtins[BUILTIN_TIME] = time;
    m_builtins[BUILTIN_SAMPLE_RATE] = sample_rate;
}

void Evaluator::update(double time)
{
    m_builtins[BUILTIN_TIME] = time;
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
                eval_error.message = make_string("String literals aren't used and doesn't mean anything yet");
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

            eval_error.message = make_string("Variable not defined"); // @todo error message
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
                    eval_error.message = make_string("Invalid binary operator"); // @todo maybe this should be considered a bug case
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
            auto call = static_cast<Expr_Call*>(expr);
            if (is_builtin_function(call))
            {
                Eval arg = evaluate(call->arguments.data[0]);
                if (!arg.success)
                {
                    return fail;
                }

                return { m_builtin_functions[call->fn_id](arg.value), true };
            }
            else {
                NOT_IMPLEMENTED("User defined functions")
            }
        }
    }

    return fail;
}

// @todo string builder here
void print_expr(Expr* expr, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        printf("    ");
    }

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
            printf("Variable %s\n", var->name.data);
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
            auto call = static_cast<Expr_Call*>(expr);
            printf("Call %s\n", call->function_name.data);
            for (int i = 0; i < call->arguments.size; i++)
            {
                print_expr(call->arguments.get(i), indent + 1);
            }
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

// @todo cleanup

double get_sign(double x)
{
    return (x > 0) - (x < 0);
}

Function_ID get_function_id(String name)
{
    if (string_compare(name, make_string("sin"))) {
        return BUILTIN_FUNC_SIN;
    }
    else if (string_compare(name, make_string("cos"))) {
        return BUILTIN_FUNC_COS;
    }
    else if (string_compare(name, make_string("abs"))) {
        return BUILTIN_FUNC_ABS;
    }
    else if (string_compare(name, make_string("sign"))
         || string_compare(name, make_string("sgn"))) {  // @xxx is this a good idea?
        return BUILTIN_FUNC_SIGN;
    }
    else if (string_compare(name, make_string("asin"))) {
        return BUILTIN_FUNC_ARCSIN;
    }
    else if (string_compare(name, make_string("acos"))) {
        return BUILTIN_FUNC_ARCCOS;
    }

    return FUNC_ID_INVALID; // @todo user defined functions
}

void get_default_builtin_functions(Builtin_Function* list)
{
    list[BUILTIN_FUNC_SIN] = sin;
    list[BUILTIN_FUNC_COS] = cos;
    list[BUILTIN_FUNC_ABS] = fabs;
    list[BUILTIN_FUNC_SIGN] = get_sign;
    list[BUILTIN_FUNC_ARCSIN] = asin;
    list[BUILTIN_FUNC_ARCCOS] = acos;
}

bool is_builtin_function(Expr_Call* call)
{
    return call->fn_id <= BUILTIN_FUNC_COUNT;
}

Expr* collapse_expr_real(Expr* root, Builtin_Function* builtin_functions, String* error_string);
Expr* collapse_expr(Expr* root, String* error_string)
{
    bool inited = false;
    static Builtin_Function_List builtin_functions;
    if (!inited)
    {
        get_default_builtin_functions(builtin_functions);
    }

    return collapse_expr_real(root, builtin_functions, error_string);
}

// recursive depth first
Expr* collapse_expr_real(Expr* root, Builtin_Function* builtin_functions, String* error_string)
{
    Expr* expr = root;
    switch (expr->type)
    {
    case Expr_Type::Grouping:
    {
        auto group = static_cast<Expr_Grouping*>(expr);
        return collapse_expr_real(group, builtin_functions, error_string);
    }
    case Expr_Type::Binary:
    {
        auto binary = static_cast<Expr_Binary*>(expr);
        auto left = collapse_expr_real(binary->left, builtin_functions, error_string);
        auto right = collapse_expr_real(binary->right, builtin_functions, error_string);

        binary->left = left;
        binary->right = right;

        if (left->type == Expr_Type::Literal && right->type == Expr_Type::Literal)
        {
            Value left_value = static_cast<Expr_Literal*>(left)->value;
            Value right_value = static_cast<Expr_Literal*>(right)->value;

            if (left_value.type == Value::VALUE_STRING || right_value.type == Value::VALUE_STRING)
            {
                *error_string = make_string("Can't do aritmetic with strings");
                break;
            }

            double left_numeric = (left_value.type == Value::VALUE_INTEGER) ? left_value.integer : left_value.real;
            double right_numeric = (right_value.type == Value::VALUE_INTEGER) ? right_value.integer : right_value.real;

            switch (binary->op)
            {
            case Binop_Unknown:
                *error_string = make_string("Unknown binary operator");
                break;
            case Binop_Add:     return new Expr_Literal(left_numeric + right_numeric);
            case Binop_Sub:     return new Expr_Literal(left_numeric - right_numeric);
            case Binop_Mul:     return new Expr_Literal(left_numeric * right_numeric);
            case Binop_Div:     return new Expr_Literal(left_numeric / right_numeric);
            case Binop_Mod:     return new Expr_Literal(fmod(left_numeric, right_numeric));
            case Binop_Eq:      return new Expr_Literal(left_numeric == right_numeric);
            case Binop_Neq:     return new Expr_Literal(left_numeric != right_numeric);
            case Binop_Gt:      return new Expr_Literal(left_numeric > right_numeric);
            case Binop_Ge:      return new Expr_Literal(left_numeric >= right_numeric);
            case Binop_Lt:      return new Expr_Literal(left_numeric < right_numeric);
            case Binop_Le:      return new Expr_Literal(left_numeric <= right_numeric);
            }
        }
    }
    case Expr_Type::Call:
    {
        auto call = static_cast<Expr_Call*>(expr);

        bool all_literals = true;
        bool all_reals = true;
        for (int i = 0; i < call->arguments.size; i++)
        {
            call->arguments.data[i] = collapse_expr_real(call->arguments.data[i], builtin_functions, error_string);
            if (call->arguments.data[i]->type != Expr_Type::Literal)
            {
                all_literals = false;
            }
            else
            {
                if (static_cast<Expr_Literal*>(call->arguments.data[0])->value.type != Value::VALUE_REAL)
                {
                    all_reals = false;
                }
            }
        }

        if (all_literals && all_reals)
        {
            if (call->arguments.size != 1)
            {
                // @todo when you are able to define your own functions which might look like whatever it wants to this needs to be fixed
                *error_string = make_string("No known function with arity not equal to 1");
                return NULL;
            }

            Builtin_Func_Type func_type = BUILTIN_FUNC_UNKNOWN;

            if (string_compare(call->function_name, make_string("sin")))
                func_type = BUILTIN_FUNC_SIN;
            else if (string_compare(call->function_name, make_string("cos")))
                func_type = BUILTIN_FUNC_COS;
            else if (string_compare(call->function_name, make_string("abs")))
                func_type = BUILTIN_FUNC_ABS;
            else if (string_compare(call->function_name, make_string("sign")) || string_compare(call->function_name, make_string("sgn")))  // @xxx is this a good idea?
                func_type = BUILTIN_FUNC_SIGN;
            else if (string_compare(call->function_name, make_string("asin")))
                func_type = BUILTIN_FUNC_ARCSIN;
            else if (string_compare(call->function_name, make_string("acos")))
                func_type = BUILTIN_FUNC_ARCCOS;

            if (func_type == BUILTIN_FUNC_UNKNOWN)
            {
                String_Builder sb(64);
                sb.append(make_string("No such function as "));
                sb.append(call->function_name);
                *error_string = sb.to_string();  // lifetime outside of this function
                return NULL;
            }

            Expr_Literal* lit = static_cast<Expr_Literal*>(call->arguments.data[0]);
            double arg = lit->value.real;
            return new Expr_Literal(builtin_functions[func_type](arg));
        }

        break;
    }
    case Expr_Type::Literal:
    {
        return expr;
    }
    case Expr_Type::Unary:
    {
        Expr_Unary* unary = static_cast<Expr_Unary*>(expr);
        {
            auto operand = collapse_expr_real(unary->operand, builtin_functions, error_string);
            if (!operand)
                return NULL;
            unary->operand = operand;
        }

        if (unary->operand->type == Expr_Type::Literal)
        {
            auto operand = static_cast<Expr_Literal*>(unary->operand);
            switch (unary->op)
            {
                case Unop_Negate:
                {
                    if (operand->value.is_numeric())
                    {
                        *error_string = make_string("Can not negate non numeric value");
                        return NULL;
                    }

                    if (operand->value.type == Value::VALUE_INTEGER)
                    {
                        operand->value.integer = -operand->value.integer;
                    }
                    else if (operand->value.type == Value::VALUE_REAL)
                    {
                        operand->value.real = -operand->value.real;
                    }

                    return operand;
                }
                case Unop_Not:
                {
                    if (operand->value.type != Value::VALUE_BOOL)
                    {
                        *error_string = make_string("Can not apply the operator Not to non boolean value");
                        return NULL;
                    }

                    operand->value.boolean = !operand->value.boolean;
                    return operand;
                }
            }
        }

        break;
    }
    case Expr_Type::Variable:
        return expr;
    }

    return expr;
}
