#include "api.h"
#include "evaluator.h"

extern const double PI;
extern const double E;

#include <cmath>

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
            case '?':
				ADD_TOKEN(TOKEN_TYPE_QUESTION_MARK, "?")
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

bool Parser::syntax_check(String expression_string) {
	auto save_symbols = symbols;
	symbols = Array<Variable>();
	Expr* expression = parse(expression_string);
	symbols = save_symbols;

	if (expression) {
		delete expression;
		return true;
	}
	else {
		return false;
	}
}

bool Parser::check_expression_string(String expression_string) {
  Expr* expression = parse(expression_string);

  if (expression) {
    delete expression;
    return true;
  }
  else {
    return false;
  }
}


Expr* Parser::parse(String expression_string)
{
    tokens = tokenize(expression_string);

    Expr* expression = nullptr;

    expression = parse_expression();

    if (tokens.get(cursor).type != TOKEN_TYPE_END)
    {
        if (expression) {
            parser_error = Error(make_string("Trailing tokens in expression"), cursor);
        }

        if (expression)
            delete expression;

        return nullptr;
    }

    return expression;
}

Expr* Parser::parse_expression()
{
    Expr* expr = parse_ternary_expr();
    if (expr)
    {
        String error_string = {};
        expr = collapse_expr(expr, &error_string);

        if (!expr)
        {
            fprintf(stderr, "Collapse expression failed\n");
			fprintf(stderr, "%s\n", error_string.data);
        }
    }

    return expr;
}

Expr* Parser::parse_ternary_expr() {
	Expr* expr = parse_equality_expr();
	if (!expr) return nullptr;

	if (tokens.get(cursor).type == TOKEN_TYPE_QUESTION_MARK) {
		cursor++;  // ?

		Expr* then_branch = parse_expression();
		if (tokens.get(cursor).type != TOKEN_TYPE_COLON) {
			free_tree(expr);
			free_tree(then_branch);
			return nullptr;
		}

		cursor++;  // :

		Expr* else_branch = parse_expression();
		if (!(then_branch && else_branch)) {
			free_tree(expr);
			free_tree(then_branch);
			free_tree(else_branch);
			return nullptr;
		}

		return new Expr_Ternary(expr, then_branch, else_branch);
	}
	else {
		return expr;
	}
}

Expr* Parser::parse_equality_expr()
{
    Expr* left = parse_comparison_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_EQUALS_EQUALS || type == TOKEN_TYPE_EXCLAMATION_EQUALS)
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
    while (type == TOKEN_TYPE_GREATER || type == TOKEN_TYPE_LESS)
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
    Expr* left = parse_mod_expr();
    Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_STAR || type == TOKEN_TYPE_SLASH)
    {
        cursor++;

        Expr* right = parse_mod_expr();
        if (!right) {
			free_tree(left);
            return NULL;
        }

        Op_Binary op = get_binop(type);
		ASSERT(op != Binop_Unknown);

        left = new Expr_Binary(left, right, op);

        type = tokens.get(cursor).type;
    }

    return left;
}

Expr* Parser::parse_mod_expr() {
	Expr* left = parse_unary_expr();

	Token_Type type = tokens.get(cursor).type;
    while (type == TOKEN_TYPE_PERCENT)
	{
		cursor++;

		Expr* right = parse_unary_expr();
		if (!right) {
			free_tree(left);
			return NULL;
		}

		// Op_Binary op = Binop_Mod;
		Op_Binary op = get_binop(type);
		ASSERT(op != Binop_Unknown);

		left = new Expr_Binary(left, right, op);

		type = tokens.get(cursor).type;
	}

	return left;
}

Expr* Parser::parse_unary_expr()
{
    Token_Type type = tokens.get(cursor).type;
    Expr* operand = NULL;
    while (type == TOKEN_TYPE_MINUS || type == TOKEN_TYPE_EXCLAMATION)
    {
        cursor++;

        operand = parse_unary_expr();
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

    DArray<Expr*> arguments;

    int num_open_parens = 1;
    while (num_open_parens != 0)
    {
        if (tokens.get(cursor).type == TOKEN_TYPE_END) {
            parser_error = Error(make_string("Missing ')'"), tokens.get(cursor).offset);

            arguments.free();
            return NULL;
        }

        Expr* expression = parse_expression();
        if (!expression) {
            arguments.free();
            return NULL;
        }

        arguments.add(expression);

        if (tokens.get(cursor).type != TOKEN_TYPE_COMMA && tokens.get(cursor).type != TOKEN_TYPE_PAREN_CLOSE) {
            parser_error = Error(make_string("Expected ',' to seperate or otherwise ')' to close argument list to function call"), tokens.get(cursor).offset);
            arguments.free();

            return NULL;
        }

        if (tokens.get(cursor).type == TOKEN_TYPE_COMMA) {
            cursor += 1;
        }

        if (tokens.get(cursor).type == TOKEN_TYPE_PAREN_CLOSE) {
            num_open_parens -= 1;
        }
        else if (tokens.get(cursor).type == TOKEN_TYPE_PAREN_OPEN) {
            num_open_parens += 1;
        }
    }

    ASSERT(tokens.get(cursor).type == TOKEN_TYPE_PAREN_CLOSE);
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
            BuiltinVar_ID builtin_var_id = get_builtin_var_id(token.token_string);
            double builtin_constant = get_builtin_constant(token.token_string);
            if (builtin_var_id != BUILTIN_VAR_ID_INVALID) {
                // the variable is a builtin

                switch (builtin_var_id) {
                    case BUILTIN_VARIABLE_TIME:  // fallthrough
                    case BUILTIN_VARIABLE_SAMPLE_RATE: {
                        return new Expr_Variable(token.token_string, builtin_var_id, Var_Type_Real);
                    }
                    case BUILTIN_VARIABLE_SAMPLE_INDEX: {
                        return new Expr_Variable(token.token_string, builtin_var_id, Var_Type_Integer);
                    }
                    case BUILTIN_VARIABLE_INPUT_SAMPLE: {
                        Expr* input_sample = new Expr_Variable(token.token_string, builtin_var_id, Var_Type_Real);
                        input_sample->flags |= EXPR_USES_INPUT_SAMPLES;

                        return input_sample;
                    }

                    default: panic("Invalid variable id");  // bug
                }
            }
            else if (builtin_constant != 0.0) {
                return new Expr_Literal(builtin_constant);
            }
            else {
                Find_Result find = find_symbol(symbols, token.token_string);

                if (find.found) {
                    Variable var = symbols.get(find.index);
                    return new Expr_Variable(var.name, find.index, var.type);
                }
                else {
                    parser_error = Error(make_string("Undefined variable"), token.offset);
                    return nullptr;
                }
            }
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

Tree_Evaluator::Tree_Evaluator()
{
    get_default_builtin_functions(builtin_functions);
}

void Tree_Evaluator::set(double sample_rate, double time)
{
    builtins[BUILTIN_VARIABLE_TIME] = time;
    builtins[BUILTIN_VARIABLE_SAMPLE_RATE] = sample_rate;
}

Eval Tree_Evaluator::evaluate_expression(Expr* expr) const
{
    Eval fail = { 0.0, false };

    switch (expr->type)
    {
        case Expr_Type::Literal:
        {
            auto literal = static_cast<Expr_Literal*>(expr);
            if (literal->value.type == Var_Type_Integer)
            {
                return { (double)literal->value.integer, true };
            }
            else if (literal->value.type == Var_Type_Real)
            {
                return { literal->value.real, true };
            }
            else if (literal->value.type == Var_Type_Boolean)
            {
                NOT_IMPLEMENTED("Logical operations")
            }
            else {
                panic("Unknown value type");  // @todo cleanup
            }
        }
        case Expr_Type::Variable:
        {
            auto var = static_cast<Expr_Variable*>(expr);

            // builtin variables
            if (is_builtin_variable(var))
            {
                return { builtins[var->var_id], true };
            }

			return { variables.get(var->var_id), true };  // panic on out of bounds
        }
        case Expr_Type::Unary:
        {
            auto unary = static_cast<Expr_Unary*>(expr);
            Eval eval = evaluate_expression(unary->operand);

            if (eval.success == false)
                return fail;

            switch (unary->op)
            {
                case Unop_Negate:
                {
                    eval.value = -eval.value;
                    return eval;
                }
                case Unop_Not:
                {
                    if (eval.value == 0) {
                        return Eval{1, true};
                    }
                    else {
                        return Eval{0, true};
                    }
                }
                default:
                    panic("Unknown unary operator");
            }
        }
        case Expr_Type::Binary:
        {
            auto binary = static_cast<Expr_Binary*>(expr);
            Eval left = evaluate_expression(binary->left);
            Eval right = evaluate_expression(binary->right);

            if (left.success == false || right.success == false)
            {
                return fail;
            }

            switch (binary->op)
            {
                // arithmetic
                case Binop_Add: return Eval{ left.value + right.value, true };
                case Binop_Sub: return Eval{ left.value - right.value, true };
                case Binop_Mul: return Eval{ left.value * right.value, true };
                case Binop_Div: return Eval{ left.value / right.value, true };
			    case Binop_Mod: return Eval{ fmod(left.value, right.value), true };

                // comparison
                case Binop_Eq: return Eval{ double(left.value == right.value), true };
                case Binop_Neq: return Eval{ double(left.value != right.value), true };
                case Binop_Gt: return Eval{ double(left.value > right.value), true };
                case Binop_Ge: return Eval{ double(left.value >= right.value), true };
                case Binop_Lt: return Eval{ double(left.value < right.value), true };
                case Binop_Le: return Eval{ double(left.value <= right.value), true };

                case Binop_Unknown: // fallthrough
                default: {
                    // eval_error.message = make_string("Invalid binary operator"); // @todo bug case
                    return fail;
                }
            }
        }
        case Expr_Type::Grouping:
        {
            auto grouping = static_cast<Expr_Grouping*>(expr);
            return evaluate_expression(grouping->expr);
        }
        case Expr_Type::Call:
        {
            auto call = static_cast<Expr_Call*>(expr);
            if (is_builtin_function(call))
            {
                Function builtin = builtin_functions[call->fn_id];

                double ret = 0.0;

                if (builtin.signature.parameter_types.size == 0) {
                    // what do we do with functions that doesn't return anything?
                }
                else if (builtin.signature.parameter_types.size == 1) {
                    Eval arg = evaluate_expression(call->arguments.data[0]);

                    if (!arg.success)
                    {
                        return fail;
                    }

                    Value argument = Value(arg.value);
                    Value result;
                    call_function(builtin, &argument, &result);

                    ret = result.real;
                }
                else {
                    static Value buffer[10];  // @todo fix
                    for (int i = 0; i < builtin.signature.parameter_types.size; i++) {
                        Eval eval = evaluate_expression(call->arguments.get(i));
                        if (!eval.success) {
                            return fail;
                        }

                        buffer[i] = Value(eval.value);
                    }

                    Value result;
                    call_function(builtin, buffer, &result);

                    ret = result.real;
                }

                return { ret, true };
            }
            else {
                NOT_IMPLEMENTED("User defined functions")
            }
        }
        case Expr_Type::Ternary: {
            auto ternary = static_cast<Expr_Ternary*>(expr);
            Eval result = evaluate_expression(ternary->condition);

            if (!result.success) return fail;

            if (result.value == 0.0) {
                return evaluate_expression(ternary->else_);
            }
            else {
                return evaluate_expression(ternary->then_);
            }
        }
        case Expr_Type::Tuple: {
            panic("We don't know how to evaluate with tuples yet");
        }
        default: {
            panic("Unknown expression type");
        }
    }
}

void print_expr(const Expr* expr, int indent);
void print_expression(const Expr* expr){
  print_expr(expr, 0);
}

void print_expr(const Expr* expr, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        printf("    ");
    }

    switch (expr->type)
    {
        case Expr_Type::Literal:
        {
            const auto literal = static_cast<const Expr_Literal*>(expr);
            switch (literal->value.type)
            {
                case Var_Type_Integer:
                    printf("Integer Literal: %li\n", literal->value.integer); break;
                case Var_Type_Real:
                    printf("Float Literal: %f\n", literal->value.real); break;
                case Var_Type_Boolean:
                    printf("Boolean Literal: %s\n", BOOL_STRING(literal->value.boolean)); break;
            }
            break;
        }
        case Expr_Type::Variable:
        {
            const auto var = static_cast<const Expr_Variable*>(expr);
            var->name.print(true);
            break;
        }
        case Expr_Type::Unary:
        {
            const auto unary = static_cast<const Expr_Unary*>(expr);
            const char* operator_string = (unary->op == Unop_Negate) ? "Negate" : "Not";
            printf("Unary Expression %s\n", operator_string);
            print_expr(unary->operand, indent + 1);
            break;
        }
        case Expr_Type::Binary:
        {
            const auto binary = static_cast<const Expr_Binary*>(expr);
            printf("Binary Expression %s\n", get_binop_string(binary->op));
            print_expr(binary->left, indent + 1);
            print_expr(binary->right, indent + 1);
            break;
        }
        case Expr_Type::Grouping:
        {
            const auto grouping = static_cast<const Expr_Grouping*>(expr);
            printf("Grouping Expression\n");
            print_expr(grouping->expr, indent + 1);
            break;
        }
        case Expr_Type::Call:
        {
            const auto call = static_cast<const Expr_Call*>(expr);
			printf("Call Expression\n");
            call->function_name.print(true);
            for (int i = 0; i < call->arguments.size; i++)
            {
                print_expr(call->arguments.get(i), indent + 1);
            }

            break;
        }
        case Expr_Type::Ternary: {
            const auto ternary = static_cast<const Expr_Ternary*>(expr);
            printf("Ternary Expression\n");
            print_expr(ternary->condition, indent + 1);
            print_expr(ternary->then_, indent + 2);
            print_expr(ternary->else_, indent + 2);

            break;
        }
        case Expr_Type::Tuple: {
            const auto tuple = static_cast<const Expr_Tuple*>(expr);
            printf("Tuple expression\n");
            for (const Expr* e : tuple->expressions) {
                print_expr(e, indent + 1);
            }

            break;
        }
        default: {
            panic("Unknown expression type");
        }
    }
}

BuiltinVar_ID get_builtin_var_id(String name)
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
    else if (string_compare(make_string("sample_index"), name) ||
             string_compare(make_string("si"), name)
            )
    {
        return BUILTIN_VARIABLE_SAMPLE_INDEX;
    }
    else if (string_compare(make_string("sample"), name) ||
             string_compare(make_string("s"), name)
            )
    {
        return BUILTIN_VARIABLE_INPUT_SAMPLE;
    }
    else
    {
        return BUILTIN_VAR_ID_INVALID;
    }
}

// returns 0 if no constant matches
double get_builtin_constant(String name) {
    if (string_compare(name, make_string("pi")) || string_compare(name, make_string("PI"))) {
        return CONSTANT_PI;
    }
    else if (string_compare(name, make_string("e")) || string_compare(name, make_string("E"))) {
        return CONSTANT_E;
    }
	else if (string_compare(name, make_string("TAU")) || string_compare(name, make_string("tau"))) {
		return CONSTANT_TAU;
	}
    else {
        return 0.0;
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
        return BUILTIN_FUNC_CLAMP;
    }
    else if (string_compare(name, make_string("exp"))) {
        return BUILTIN_FUNC_EXP;
    }
    else if (string_compare(name, make_string("pow"))) {
    	return BUILTIN_FUNC_POW;
    }

    return FUNC_ID_INVALID; // @todo user defined functions
}

void free_tree(Expr* node) {
	if (!node) {
		return;
	}

	if (node->type == Expr_Type::Binary) {
		free_tree(static_cast<Expr_Binary*>(node)->left);
		free_tree(static_cast<Expr_Binary*>(node)->right);
	}
	else if (node->type == Expr_Type::Grouping) {
		free_tree(static_cast<Expr_Grouping*>(node)->expr);
	}
	else if (node->type == Expr_Type::Unary) {
		free_tree(static_cast<Expr_Unary*>(node)->operand);
	}
	else if (node->type == Expr_Type::Call) {
		auto call = static_cast<Expr_Call*>(node);
		for (auto arg : call->arguments) {
			free_tree(arg);
		}
	}
    else if (node->type == Expr_Type::Ternary) {
        auto ternary = static_cast<Expr_Ternary*>(node);
        free_tree(ternary->condition);
        free_tree(ternary->then_);
        free_tree(ternary->else_);
    }
    else if (node->type == Expr_Type::Tuple) {
        auto tuple = static_cast<Expr_Tuple*>(node);
        for (auto expr : tuple->expressions) {
            free_tree(expr);
        }
    }

	delete node;
}

Expr* collapse_expr_real(Expr* root, Function* builtin_functions, String* error_string);
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

Expr* collapse_expr_real(Expr* root, Function* builtin_functions, String* error_string)
{
    Expr* expr = root;
    switch (expr->type)
    {
        case Expr_Type::Grouping:
		{
			auto group = static_cast<Expr_Grouping*>(expr);
			Expr* expr = group->expr;
			delete group;
			return collapse_expr_real(expr, builtin_functions, error_string);
		}
        case Expr_Type::Binary:
		{
			auto binary = static_cast<Expr_Binary*>(expr);

            ASSERT(binary->left || binary->right);

			if (!binary->left)
			{
                Expr* right = binary->right;
                delete binary;
				return collapse_expr_real(right, builtin_functions, error_string);
			}
			if (!binary->right)
			{
                Expr* left = binary->left;
                delete binary;
				return collapse_expr_real(left, builtin_functions, error_string);
			}

			auto left = collapse_expr_real(binary->left, builtin_functions, error_string);
			auto right = collapse_expr_real(binary->right, builtin_functions, error_string);

			binary->left = left;
			binary->right = right;

			if (left->type == Expr_Type::Literal && right->type == Expr_Type::Literal)
			{
				Value left_value = static_cast<Expr_Literal*>(left)->value;
				Value right_value = static_cast<Expr_Literal*>(right)->value;

                Op_Binary operation = binary->op;

                free_tree(binary);

				double left_numeric = (left_value.type == Var_Type_Integer) ? left_value.integer : left_value.real;
				double right_numeric = (right_value.type == Var_Type_Integer) ? right_value.integer : right_value.real;

                switch (operation)
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
				default:
					panic("Unknown binary operator");
				}
			}

			break;
		}
        case Expr_Type::Call:
		{
			auto call = static_cast<Expr_Call*>(expr);

			bool all_literals = true;
			bool all_reals = true;
			for (int i = 0; i < call->arguments.size; i++)
			{
				call->arguments.data[i] = collapse_expr_real(call->arguments.data[i], builtin_functions, error_string);
				if (call->arguments.data[i]->type == Expr_Type::Literal)
				{
					if (static_cast<Expr_Literal*>(call->arguments.data[i])->value.type != Var_Type_Real)
					{
						all_reals = false;
					}
				}
				else
				{
					all_literals = false;
				}
			}

			bool computable_now = is_builtin_function(call) && all_literals && all_reals;

			if (is_builtin_function(call))
			{
                Function builtin = builtin_functions[call->fn_id];

				if (call->arguments.size != builtin.signature.parameter_types.size) {
					*error_string = make_string("Wrong number of arguments");
					return NULL;
				}

				// @todo proper typechecking for arguments

				int arg_count = call->arguments.size;
				int ret_count = builtin.signature.return_types.size;

				if (arg_count == 1 && computable_now) {
					call->arguments.data[0] = collapse_expr_real(call->arguments.data[0], builtin_functions, error_string);

					if (call->arguments.get(0)->type == Expr_Type::Literal) {
						Expr_Literal* lit = static_cast<Expr_Literal*>(call->arguments.get(0));

						if (ret_count == 1) {
							Value arg = Value(lit->value);
							Value value;
							call_function(builtin, &arg, &value);
							free_tree(call);
							return new Expr_Literal(value);
						}
					}
				}

				if (computable_now)
				{
					DArray<Value> arguments(arg_count);
					for (int i = 0; i < arg_count; i++) {
						// we know that all of them should be literals
						ASSERT(static_cast<Expr_Literal*>(call->arguments.get(i))->type == Expr_Type::Literal);
						Value value = static_cast<Expr_Literal*>(call->arguments.get(i))->value;
						arguments.add(value);
					}

					Expr* ret = nullptr;

					if (builtin.signature.return_types.size == 0) {
						// @todo do we actually allow for functions that don't return anything.
						// the current assumption is that since they are signal generators, all functions are pure.
						// so it wouldn't make sense for a function to not return anything and currently no such functions exist.
						// but if we decide to add them how their semantics should be depends on what exactly they are or what exactly they do.
						// so we can't decide on that.

						// if they mess up with some settings to the system like setting some global variables or settings only once then we can safely only "execute" them once and not put them on the tree.
						// but magic functions that mess with settings sounds like a horrible idea so we will probably never add them.
						// if they do something per sample then we should put them to the tree and if they ever exist.
						// the second one is the more probable future hence the active path instead of the commented out one.

						return call;
						// call_function(builtin, arguments.data(), nullptr);
					}
					else if (builtin.signature.return_types.size == 1) {
						Value result = {};
						call_function(builtin, arguments.data(), &result);
						ret = new Expr_Literal(result);
					}
					else {
						DArray<Value> res (builtin.signature.return_types.size);
						call_function(builtin, arguments.data(), res.data());

						DArray<Expr*> return_expressions(ret_count);
						for (int i = 0; i < ret_count; i++) {
							return_expressions.get_ref(i) = new Expr_Literal(res.get(i));
						}

						res.reset();

						ret = new Expr_Tuple(Array<Expr*>(return_expressions));
					}

					arguments.reset();
					free_tree(call);
					return ret;
				}
				else {
					return call;
				}
			}
			else {
				panic("User defined functions not implemented");
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
                Op_Unary unop = unary->op;
				auto operand = static_cast<Expr_Literal*>(unary->operand);

                delete unary;

				switch (unop)
				{
                case Unop_Negate:
					{
						if (!operand->value.is_numeric())
						{
							*error_string = make_string("Can not negate non numeric value");
							return NULL;
						}

						if (operand->value.type == Var_Type_Integer)
						{
							operand->value.integer = -operand->value.integer;
						}
						else if (operand->value.type == Var_Type_Real)
						{
							operand->value.real = -operand->value.real;
						}

						return operand;
					}
                case Unop_Not:
					{
						if (operand->value.type != Var_Type_Boolean)
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
		{
			auto var = static_cast<Expr_Variable*>(expr);
			if (var->var_id == BUILTIN_VAR_ID_INVALID)
			{
				*error_string = make_string("Undefined variable");
				return NULL;
			}
			return expr;
		}

    	case Expr_Type::Ternary:
		{
			auto ternary = static_cast<Expr_Ternary*>(expr);
			auto cond = collapse_expr_real(ternary->condition, builtin_functions, error_string);
			if (!cond) {
				return NULL;
			}

			if (cond->type == Expr_Type::Literal) {
				bool thruth_value = static_cast<Expr_Literal*>(cond)->value.evaluate_truth_value();

				Expr* path = nullptr;
				if (thruth_value) {
					path = collapse_expr_real(ternary->then_, builtin_functions, error_string);
					ternary->then_ = nullptr;
				}
				else {
					path = collapse_expr_real(ternary->else_, builtin_functions, error_string);
					ternary->else_ = nullptr;
				}

				free_tree(ternary);

				return path;
			}

			ternary->condition = cond;

			auto then_ = collapse_expr_real(ternary->then_, builtin_functions, error_string);
			auto else_ = collapse_expr_real(ternary->else_, builtin_functions, error_string);

			if (!(then_ && else_)) {
				return NULL;
			}

			ternary->then_ = then_;
			ternary->else_ = else_;

			return ternary;
		}
        case Expr_Type::Tuple:
        {
            auto tuple = static_cast<Expr_Tuple*>(expr);
            for (auto& e : tuple->expressions) {
                e = collapse_expr_real(e, builtin_functions, error_string);
                if (!e)
                {
                    free_tree(tuple);
                    return NULL;
                }
            }
            return tuple;
        }
    }
    return expr;
}
