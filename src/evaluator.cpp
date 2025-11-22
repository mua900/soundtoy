#include "evaluator.h"

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
                else if (ch == '"')
                {
                    int start = cursor + 1;
                    cursor++;
                    while (ch != '"')
                    {
                        if (cursor >= expression.size)
                        {
                            fprintf(stderr, "Unterminated string literal\n");
                            break;
                        }
                        cursor++;
                        ch = expression.data[cursor];
                    }

                    String s_literal = string_slice(expression, start, cursor);
                    tokens.add(Token(s_literal, TOKEN_TYPE_LITERAL_STRING, start));
                }
                else {
                    fprintf(stderr, "Unknown character %c", ch);
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
            fprintf(stderr, "Malformed token stream\n");
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
            fprintf(stderr, "Collapse expression failed\n");
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
    if (!(tokens.get(cursor).type == TOKEN_TYPE_IDENT && tokens.get(cursor + 1).type == TOKEN_TYPE_PAREN_OPEN))
    {
        return parse_primary_expr();
    }

    String name = tokens.get(cursor).token_string;
    cursor++;  // identifier

    // function call
    cursor++;  // (

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

    Function_ID fn_id = get_function_id(name);
    if (fn_id == FUNC_ID_INVALID)
    {
        return NULL;
    }

    return new Expr_Call(name, Array<Expr*>(arguments), fn_id);
}

Expr* Parser::parse_primary_expr()
{
    Token token = tokens.get(cursor);
    Token_Type type = token.type;
    switch (type)
    {
        case TOKEN_TYPE_LITERAL_INT:
        {
            cursor++;
            bool success = false;
            long long i = string_to_integer(token.token_string, &success);
            return new Expr_Literal(i);
        }
        case TOKEN_TYPE_LITERAL_FLOAT:
            cursor++;
            return new Expr_Literal(string_to_real(token.token_string));
        case TOKEN_TYPE_IDENT:
        {
            cursor++;
            Var_ID var_id = get_var_id(token.token_string);
            return new Expr_Variable(token.token_string, var_id);
        }
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

const char* Builtin_Func_Names[BUILTIN_FUNC_COUNT] = {
    "BUILTIN_FUNC_SIN",
    "BUILTIN_FUNC_COS",
    "BUILTIN_FUNC_ABS",
    "BUILTIN_FUNC_SIGN",
    "BUILTIN_FUNC_ARCSIN",
    "BUILTIN_FUNC_ARCCOS",
};


Evaluator::Evaluator()
{
    get_default_builtin_functions(builtin_functions);
}

void Evaluator::set(double sample_rate, double time)
{
    builtins[BUILTIN_VARIABLE_TIME] = time;
    builtins[BUILTIN_VARIABLE_SAMPLE_RATE] = sample_rate;
}

void Evaluator::update(double time)
{
    builtins[BUILTIN_VARIABLE_TIME] = time;
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
            if (literal->value.type == Value_Type::INTEGER)
            {
                // @todo integer math
                return { (double)literal->value.integer, true };
            }
            else if (literal->value.type == Value_Type::REAL)
            {
                return { literal->value.real, true };
            }
            else if (literal->value.type == Value_Type::BOOL)
            {
                NOT_IMPLEMENTED("Logical operations")
            }
            else
                panic("Unknown value type");  // @todo cleanup
        }
        case Expr_Type::Variable:
        {
            auto var = static_cast<Expr_Variable*>(expr);

            // builtin variables
            if (is_builtin_variable(var))
            {
                return { builtins[var->var_id], true };
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
                    eval_error.message = make_string("Invalid binary operator"); // @todo bug case
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

                float result = builtin_functions[call->fn_id](arg.value);

                return { result, true };
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
                case Value_Type::INTEGER:
                    printf("Literal: %lli\n", literal->value.integer); break;
                case Value_Type::REAL:
                    printf("Literal: %f\n", literal->value.real); break;
                case Value_Type::BOOL:
                    printf("Bool: %s\n", BOOL_STRING(literal->value.boolean)); break;
            }
            break;
        }
        case Expr_Type::Variable:
        {
            auto var = static_cast<Expr_Variable*>(expr);
            var->name.print(true);
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
            call->function_name.print(true);
            for (int i = 0; i < call->arguments.size; i++)
            {
                print_expr(call->arguments.get(i), indent + 1);
            }
        }
    }
}

// @todo builtin constants
Var_ID get_var_id(String name)
{
    if (string_compare(make_string("time"), name) ||
        string_compare(make_string("t"), name)
        )
    {
        return BUILTIN_VARIABLE_TIME;
    }
    else if (string_compare(make_string("sample_rate"), name) ||
             string_compare(make_string("sr"), name)
            )
    {
        return BUILTIN_VARIABLE_SAMPLE_RATE;
    }
    else
    {
        return VAR_ID_INVALID;
    }
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
         || string_compare(name, make_string("sgn"))) {
        return BUILTIN_FUNC_SIGN;
    }
    else if (string_compare(name, make_string("asin"))) {
        return BUILTIN_FUNC_ARCSIN;
    }
    else if (string_compare(name, make_string("acos"))) {
        return BUILTIN_FUNC_ARCCOS;
    }
    else if (string_compare(name, make_string("ceil"))) {
        return BUILTIN_FUNC_CEIL;
    }
    else if (string_compare(name, make_string("floor"))) {
        return BUILTIN_FUNC_FLOOR;
    }
    else if (string_compare(name, make_string("smoothstep"))) {
        return BUILTIN_FUNC_SMOOTHSTEP;
    }
    else if (string_compare(name, make_string("clamp"))) {
        return BUILTIN_FUNC_CLAMP_RANGE_NORMAL;
    }
    else if (string_compare(name, make_string("clamp_audio"))) {  // @todo get rid
        return BUILTIN_FUNC_CLAMP_RANGE_AUDIO;
    }

    return FUNC_ID_INVALID; // @todo user defined functions
}

Expr* collapse_expr_real(Expr* root, Builtin_Function* builtin_functions, String* error_string);
Expr* collapse_expr(Expr* root, String* error_string)
{
    static bool inited = false;
    static Builtin_Function_List builtin_functions;
    if (!inited)
    {
        get_default_builtin_functions(builtin_functions);
        inited = true;
    }

    return collapse_expr_real(root, builtin_functions, error_string);
}

// @todo it might not be the best idea to do optimizations on the tree as we do here.

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

        if (!binary->left)
        {
            return collapse_expr_real(binary->right, builtin_functions, error_string);
        }
        if (!binary->right)
        {
            return collapse_expr_real(binary->left, builtin_functions, error_string);
        }

        auto left = collapse_expr_real(binary->left, builtin_functions, error_string);
        auto right = collapse_expr_real(binary->right, builtin_functions, error_string);

        binary->left = left;
        binary->right = right;

        if (left->type == Expr_Type::Literal && right->type == Expr_Type::Literal)
        {
            Value left_value = static_cast<Expr_Literal*>(left)->value;
            Value right_value = static_cast<Expr_Literal*>(right)->value;

            double left_numeric = (left_value.type == Value_Type::INTEGER) ? left_value.integer : left_value.real;
            double right_numeric = (right_value.type == Value_Type::INTEGER) ? right_value.integer : right_value.real;

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

        break;
    }
    case Expr_Type::Call:
    {
        auto call = static_cast<Expr_Call*>(expr);

        // @todo typechecking for arguments

        bool all_literals = true;
        bool all_reals = true;
        for (int i = 0; i < call->arguments.size; i++)
        {
            call->arguments.data[i] = collapse_expr_real(call->arguments.data[i], builtin_functions, error_string);
            if (call->arguments.data[i]->type == Expr_Type::Literal)
            {
                if (static_cast<Expr_Literal*>(call->arguments.data[i])->value.type != Value_Type::REAL)
                {
                    all_reals = false;
                }
            }
            else
            {
                all_literals = false;
            }
        }

        if (is_builtin_function(call) && all_literals && all_reals)
        {
            if (call->arguments.size != 1)
            {
                *error_string = make_string("Builtin functions take 1 parameter");
                return NULL;
            }

            Expr_Literal* lit = static_cast<Expr_Literal*>(call->arguments.data[0]);
            double arg = lit->value.real;
            double bake_value = builtin_functions[call->fn_id](arg);
            return new Expr_Literal(bake_value);
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
                    if (!operand->value.is_numeric())
                    {
                        *error_string = make_string("Can not negate non numeric value");
                        return NULL;
                    }

                    if (operand->value.type == Value_Type::INTEGER)
                    {
                        operand->value.integer = -operand->value.integer;
                    }
                    else if (operand->value.type == Value_Type::REAL)
                    {
                        operand->value.real = -operand->value.real;
                    }

                    return operand;
                }
                case Unop_Not:
                {
                    if (operand->value.type != Value_Type::BOOL)
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
        auto var = static_cast<Expr_Variable*>(expr);
        if (var->var_id == VAR_ID_INVALID)
        {
            *error_string = make_string("Undefined variable");
            return NULL;
        }
        return expr;
    }

    return expr;
}
