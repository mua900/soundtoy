#include "builtin.h"
#include <cmath>

// custom implemented builtins
double get_sign(double x);
double smoothstep(double x);

// wrappers
void st_fabs(Value* parameters, Value* results);
void st_get_sign(Value* parameters, Value* results);
void st_ceil(Value* parameters, Value* results);
void st_floor(Value* parameters, Value* results);
void st_smoothstep(Value* parameters, Value* results);
void st_clamp(Value* parameters, Value* results);
void st_sin(Value* parameters, Value* results);
void st_cos(Value* parameters, Value* results);
void st_tan(Value* parameters, Value* results);
void st_asin(Value* parameters, Value* results);
void st_acos(Value* parameters, Value* results);
void st_atan(Value* parameters, Value* results);
void st_exp(Value* parameters, Value* results);
void st_pow(Value* parameters, Value* results);
void st_log(Value* parameters, Value* results);
void st_fract(Value* parameters, Value* results);
void st_mix(Value* parameters, Value* results);
void st_saw(Value* parameters, Value* results);
void st_square(Value* parameters, Value* results);
void st_triangle(Value* parameters, Value* results);

void call_function(Function func, Value* parameters, Value* results) {
    func.implementation(parameters, results);
}

void get_default_builtin_functions(Function* list)
{
    static Variable_Type single_value[1] = { Var_Type_Real };
    static Variable_Type two_values[2] = { Var_Type_Real, Var_Type_Real };
    static Variable_Type three_values[3] = { Var_Type_Real, Var_Type_Real, Var_Type_Real };

    list[BUILTIN_FUNC_ABS] = Function(
        st_fabs,
        FunctionSignature(String("abs"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SIGN] = Function(
        st_get_sign,
        FunctionSignature(String("sign"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_CEIL] = Function(
        st_ceil,
        FunctionSignature(String("ceil"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_FLOOR] = Function(
        st_floor,
        FunctionSignature(String("floor"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SIN] = Function(
        st_sin,
        FunctionSignature(String("sin"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_COS] = Function(
        st_cos,
        FunctionSignature(String("cos"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_ARCSIN] = Function(
        st_asin,
        FunctionSignature(String("asin"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_ARCCOS] = Function(
        st_acos,
        FunctionSignature(String("acos"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_EXP] = Function(
        st_exp,
        FunctionSignature(String("exp"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SMOOTHSTEP] = Function(
        st_smoothstep,
        FunctionSignature(String("smoothstep"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_CLAMP] = Function(
        st_clamp,
        FunctionSignature(String("clamp"), make_array(single_value), make_array(three_values))
    );
    list[BUILTIN_FUNC_POW] = Function(
    	st_pow,
    	FunctionSignature(String("pow"), make_array(single_value), make_array(two_values))
    );
    list[BUILTIN_FUNC_FRACT] = Function(
    	st_fract,
    	FunctionSignature(String("fract"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_MIX] = Function(
        st_mix,
        FunctionSignature(String("mix"), make_array(single_value), make_array(three_values))
    );
    list[BUILTIN_FUNC_SAW] = Function(
        st_saw,
        FunctionSignature(String("saw"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SQUARE] = Function(
        st_square,
        FunctionSignature(String("square"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_TRIANGLE] = Function(
        st_triangle,
        FunctionSignature(String("triangle"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_TAN] = Function(
        st_tan,
        FunctionSignature(String("tan"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_ARCTAN] = Function(
        st_atan,
        FunctionSignature(String("atan"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_LOG] = Function(
        st_log,
        FunctionSignature(String("log"), make_array(single_value), make_array(single_value))
    );
}

bool is_builtin_function(const Expr_Call* call)
{
    return call->fn_id < BUILTIN_FUNC_COUNT;
}

bool is_builtin_variable(const Expr_Variable* var)
{
    return var->var_id < BUILTIN_VARIABLE_COUNT;
}

double fract(double x) {
	return x - floor(x);
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

double clamp(double x, double lower, double upper) {
    return CLAMP(x, lower, upper);
}

double mix(double a, double b, double t) {
    return (1.0 - t) * a + t * b;
}

double saw(double x) {
    return 2 * (x - floor(x + 0.5));
}

double square(double x) {
    return floor(x) * 4 - floor(2 * x) * 2 + 1;
}

double triangle(double x) {
    return 4 * abs(x - floor(x + 0.75) + 0.25) - 1;  // @todo this is wrong
}

// wrappers

void st_fabs(Value* parameters, Value* results) { results[0] = Value(fabs(parameters[0].real)); }
void st_get_sign(Value* parameters, Value* results) { results[0] = Value(get_sign(parameters[0].real)); }
void st_ceil(Value* parameters, Value* results) { results[0] = Value(ceil(parameters[0].real)); }
void st_floor(Value* parameters, Value* results) { results[0] = Value(floor(parameters[0].real)); }
void st_sin(Value* parameters, Value* results) { results[0] = Value(sin(parameters[0].real)); }
void st_cos(Value* parameters, Value* results) { results[0] = Value(cos(parameters[0].real)); }
void st_tan(Value* parameters, Value* results) { results[0] = Value(tan(parameters[0].real)); }
void st_asin(Value* parameters, Value* results) { results[0] = Value(asin(parameters[0].real)); }
void st_acos(Value* parameters, Value* results) { results[0] = Value(acos(parameters[0].real)); }
void st_atan(Value* parameters, Value* results) { results[0] = Value(atan(parameters[0].real)); }
void st_exp(Value* parameters, Value* results) { results[0] = Value(exp(parameters[0].real)); }
void st_log(Value* parameters, Value* results) { results[0] = Value(log(parameters[0].real)); }
void st_smoothstep(Value* parameters, Value* results) { results[0] = Value(smoothstep(parameters[0].real)); }
void st_clamp(Value* parameters, Value* results) { results[0] = Value(clamp(parameters[0].real, parameters[1].real, parameters[2].real)); }
void st_pow(Value* parameters, Value* results) { results[0] = Value(pow(parameters[0].real, parameters[1].real)); }
void st_fract(Value* parameters, Value* results) { results[0] = Value(fract(parameters[0].real)); }
void st_mix(Value* parameters, Value* results) { results[0] = Value(mix(parameters[0].real, parameters[1].real, parameters[2].real)); }
void st_saw(Value* parameters, Value* results) { results[0] = Value(saw(parameters[0].real)); }
void st_square(Value* parameters, Value* results) { results[0] = Value(square(parameters[0].real)); }
void st_triangle(Value* parameters, Value* results) { results[0] = Value(triangle(parameters[0].real)); }