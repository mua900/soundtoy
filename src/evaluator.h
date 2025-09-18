#pragma once

#include "common.h"
#include "template.h"

#include "token.h"
#include "expr.h"

DArray<Token> tokenize(String expression);

// @todo turn this into a something that can register and remember errors
struct Error {
    String message = {};
    int offset = 0;
};

struct Parser {
    Parser() {}

    int cursor = 0;
    Error parser_error = {};

    Array<Expr*> parse(String expression);

private:
    Array<Token> tokens;

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

using Builtin_Function = double (*) (double);

using Function_ID = unsigned int;

enum Builtin_Func_Type {
    BUILTIN_FUNC_SIN,
    BUILTIN_FUNC_COS,
    BUILTIN_FUNC_ABS,
    BUILTIN_FUNC_SIGN,
    BUILTIN_FUNC_ARCSIN,
    BUILTIN_FUNC_ARCCOS,

    BUILTIN_FUNC_COUNT,

    BUILTIN_FUNC_UNKNOWN,
};

#define FUNC_ID_INVALID (Function_ID)(-1)

Function_ID get_function_id(String name);

enum Builtin_Type {
    BUILTIN_TIME = 0,
    BUILTIN_SAMPLE_RATE,

    BUILTIN_COUNT
};

struct Eval {
    double value;
    bool success;
};

using Builtin_Function_List = Builtin_Function[BUILTIN_FUNC_COUNT];

// the argument must be an array of pointers of size BUILTIN_FUNC_COUNT
void get_default_builtin_functions(Builtin_Function* func_list);

bool is_builtin_function(Expr_Call* call);

struct Evaluator
{
    void set(double sample_rate, double time);
    void update(double time);

    Eval evaluate(Expr* expr);

    Builtin_Function_List m_builtin_functions = {};
    double m_builtins[BUILTIN_COUNT] = {};
    Error eval_error = {};
};

// collapse the expression (constant fold) and return the new root node of the collapsed expression
// does typechecking in the process
// set the error and returns null on failure
Expr* collapse_expr(Expr* root, String* error_string);
