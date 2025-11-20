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

    BUILTIN_FUNC_COUNT,

    BUILTIN_FUNC_UNKNOWN,
};

enum Builtin_Variable {
    // constant per evaluation
    BUILTIN_TIME = 0,
    BUILTIN_SAMPLE_RATE,

    // constant
    BUILTIN_CONST_PI,
    BUILTIN_CONST_E,

    BUILTIN_COUNT
};

using Builtin_Function_List = Builtin_Function[BUILTIN_FUNC_COUNT];

// the argument must be an array of pointers of size BUILTIN_FUNC_COUNT
void get_default_builtin_functions(Builtin_Function* func_list);

bool is_builtin_function(Expr_Call* call);
