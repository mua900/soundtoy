emcc -O3 ..\core\api.cpp ..\core\bytecode.cpp ..\core\evaluator.cpp ..\core\builtin.cpp ..\core\common.cpp ..\core\expr.cpp `
    -sEXPORTED_RUNTIME_METHODS="[ccall, cwrap]" -sALLOW_MEMORY_GROWTH=1 -sWASM=1 `
    -sEXPORT_NAME="SoundtoyCore" -sEXPORTED_FUNCTIONS="[_st_initialize,_st_sampler_create,_st_sampler_destroy,_st_check_expression_string,_st_sampler_set_expression,_st_sampler_evaluate,_st_sampler_step_time,_st_sampler_set_sample_rate,_st_sampler_set_sample_time,_st_sampler_get_sample_rate,_st_sampler_get_sample_time,_st_sampler_register_variable,_st_sampler_set_variable_value,_st_sampler_get_variable_value,_st_sampler_get_variable_name_at_index,_st_sampler_get_variable_count,_st_fill,_st_fill_interleaved]" `
    -o soundtoy.js
