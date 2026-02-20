emcc -O3 ../st/evaluation/api.cpp ../st/evaluation/bytecode.cpp ../st/evaluation/evaluator.cpp ../st/evaluation/builtin.cpp ../st/evaluation/expr.cpp ../st/common/common.cpp \
	-I ../st/common \
    -sEXPORTED_RUNTIME_METHODS="[ccall, cwrap, stringToUTF8]" -sALLOW_MEMORY_GROWTH=1 -sWASM=1                                \
    -sEXPORT_NAME="st_lib"                                                                                      \
    -sEXPORTED_FUNCTIONS="[_st_initialize,_st_get_last_error,_st_sampler_create,_st_sampler_destroy,_st_sampler_copy,_st_check_expression_string,_st_sampler_set_expression,_st_sampler_evaluate,_st_sampler_step_time,_st_sampler_set_sample_rate,_st_sampler_set_sample_time,_st_sampler_get_sample_rate,_st_sampler_get_sample_time,_st_sampler_register_variable,_st_sampler_set_variable_value,_st_sampler_get_variable_value,_st_sampler_get_variable_name_at_index,_st_sampler_get_variable_count,_st_set_input_stream,_st_clear_input_stream,_st_fill,_st_fill_interleaved,_st_fill_strided,_st_fill_planar,  \
    _malloc, _free]" -std=c++20 \
    -o soundtoy.js
