#pragma once

#include "expr.h"

using Builtin_Function = double (*) (double);

enum Builtin_Func_Type {
    BUILTIN_FUNC_SIN,
    BUILTIN_FUNC_COS,
    BUILTIN_FUNC_ABS,
    BUILTIN_FUNC_SIGN,
    BUILTIN_FUNC_ARCSIN,
    BUILTIN_FUNC_ARCCOS,

    // @todo
    /*
    CEIL,
    FLOOR,
    SMOOTHSTEP,
    */

    BUILTIN_FUNC_COUNT,

    BUILTIN_FUNC_UNKNOWN,
};

enum Builtin_Variable : int {
    // constant per evaluation
    BUILTIN_VARIABLE_TIME = 0,
    BUILTIN_VARIABLE_SAMPLE_RATE,

    BUILTIN_VARIABLE_COUNT
};

enum Builtin_Constants {
    // constant
    BUILTIN_CONSTANT_PISS,
    BUILTIN_CONSTANT_ESS,

    BUILTIN_CONSTANT_COUNT,
};

using Builtin_Function_List = Builtin_Function[BUILTIN_FUNC_COUNT];

// the argument must be an array of pointers of size BUILTIN_FUNC_COUNT
void get_default_builtin_functions(Builtin_Function* func_list);

bool is_builtin_function(const Expr_Call* call);
bool is_builtin_variable(const Expr_Variable* var);