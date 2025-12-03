#pragma once

#include "common.h"
#include "template.h"

#include "builtin.h"
#include "token.h"
#include "expr.h"
#include "bytecode.h"

DArray<Token> tokenize(String expression);

struct Error {
    String message = {};
    int offset = 0;

    Error() {}
    Error(String msg, int off) : message(msg), offset(off) {}
};

struct Parser {
    Parser() {}

    Expr* parse(String expression);

    void set_symbols(Array<String> p_symbols) { symbols = p_symbols; }

    // @todo
    bool syntax_check(String expression);
    bool check_expression_string(String expression, Array<String> symbols);

    Error get_error() const { return parser_error; }

private:
    Array<String> symbols = {};
    Array<Token> tokens;

    int cursor = 0;
    Error parser_error = {};

    Expr* parse_expression();

    // according to precedence in order
    Expr* parse_equality_expr();
    Expr* parse_comparison_expr();
    Expr* parse_arithmetic_expr();
    Expr* parse_factor_expr();
    Expr* parse_unary_expr();
    Expr* parse_call_expr();
    Expr* parse_primary_expr();

    bool consume(Token_Type type);

    void report_error();  // @todo
};

using BuiltinVar_ID = unsigned int;
using Function_ID = unsigned int;

#define BUILTIN_VAR_ID_INVALID (BuiltinVar_ID)(-1)
#define FUNC_ID_INVALID (Function_ID)(-1)

// @todo user defined variables and functions
BuiltinVar_ID get_builtin_var_id(String name);
Function_ID get_function_id(String name);
double get_builtin_constant(String name);  // returns 0.0 if no constant name matches

struct Eval {
    double value = 0.0;
    bool success = false;
};

// on tree evaluator
struct Tree_Evaluator
{
    Tree_Evaluator();

    Expr* expression = nullptr;

    DArray<float> variables = {};
    DArray<String> symbols = {};

    Builtin_Function_List builtin_functions = {};
    double builtins[BUILTIN_VARIABLE_COUNT] = {};
    Error eval_error = {};

    int add_symbol(String symbol) {
        symbols.add(symbol);
        return variables.add(0.0);
    }

    void set(double sample_rate, double time);
    void update(double time);

    Eval evaluate() const { return evaluate_expression(expression); }
    Eval evaluate_expression(Expr* expr) const;

    void reset(double sample_rate, double time) {
        set(sample_rate, time);
        eval_error = Error();
    }

    double get_time() { return builtins[BUILTIN_VARIABLE_TIME]; }
    double get_sample_rate() { return builtins[BUILTIN_VARIABLE_SAMPLE_RATE]; }

    void step_time(double step) { builtins[BUILTIN_VARIABLE_TIME] += step; }
};

// collapse the expression (constant fold) and return the new root node of the collapsed expression
// does typechecking in the process
// sets the error and returns null on failure
Expr* collapse_expr(Expr* root, String* error_string);
