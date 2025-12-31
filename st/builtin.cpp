#include "builtin.h"
#include <cmath>

// custom implemented builtins
double get_sign(double x);
double smoothstep(double x);
double clamp_range_normal(double x);
double clamp_range_audio(double x);  // @todo get rid

void get_default_builtin_functions(Builtin_Function* list)
{
    list[BUILTIN_FUNC_ABS] = fabs;
    list[BUILTIN_FUNC_SIGN] = get_sign;
    list[BUILTIN_FUNC_CEIL] = ceil;
    list[BUILTIN_FUNC_FLOOR] = floor;
    list[BUILTIN_FUNC_SMOOTHSTEP] = smoothstep;
    list[BUILTIN_FUNC_CLAMP_RANGE_NORMAL] = clamp_range_normal;
    list[BUILTIN_FUNC_CLAMP_RANGE_AUDIO] = clamp_range_audio;
    list[BUILTIN_FUNC_SIN] = sin;
    list[BUILTIN_FUNC_COS] = cos;
    list[BUILTIN_FUNC_ARCSIN] = asin;
    list[BUILTIN_FUNC_ARCCOS] = acos;
    list[BUILTIN_FUNC_EXP] = exp;
}

bool is_builtin_function(const Expr_Call* call)
{
    return call->fn_id <= BUILTIN_FUNC_COUNT;
}

bool is_builtin_variable(const Expr_Variable* var)
{
    return var->var_id <= BUILTIN_VARIABLE_COUNT;
}

double get_sign(double x)
{
    // @todo more robust version
    return (x > 0) - (x < 0);
}

double smoothstep(double x) {
    x = CLAMP(x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

double clamp_range_normal(double x) {
    return CLAMP(x, 0.0, 1.0);
}

double clamp_range_audio(double x) {
    return CLAMP(x, -1.0, 1.0);
}
