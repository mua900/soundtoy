#pragma once

extern "C" {

    enum Evaluator_Type : int {
        BYTECODE_INTERP,
        TREE_INTERP,
    };

    struct St_Sampler;

  
    bool st_initialize();
    const char* st_get_last_error();
    St_Sampler* st_sampler_create(Evaluator_Type evaluator_type, int sample_rate);
    void st_sampler_destroy(St_Sampler* sampler);
    bool st_check_expression_string(const St_Sampler* sampler_or_null, const char* expression_string, int length);
    bool st_sampler_set_expression(St_Sampler* sampler, const char* expression_string, int length);
    float st_sampler_evaluate(const St_Sampler* sampler);
    void st_sampler_step_time(St_Sampler* sampler, float step_size);
    void st_sampler_set_sample_rate(St_Sampler* sampler, float sample_rate);
    void st_sampler_set_sample_time(St_Sampler* sampler, float sample_time);
    float st_sampler_get_sample_rate(const St_Sampler* sampler);
    float st_sampler_get_sample_time(const St_Sampler* sampler);
    int st_sampler_register_variable(St_Sampler* sampler, const char* name, int length);
    bool st_sampler_set_variable_value(St_Sampler* sampler, int variable, float value);
    float st_sampler_get_variable_value(const St_Sampler* sampler, int variable);
    const char* st_sampler_get_variable_name_at_index(const St_Sampler* sampler, int index);
    int st_sampler_get_variable_count(const St_Sampler* sampler);
    void st_fill(St_Sampler* sampler, float* buffer, int length);
	void st_fill_strided(St_Sampler* sampler, float* buffer, int sample_count);
    void st_fill_interleaved(St_Sampler* sampler_left, St_Sampler* sampler_right, float* buffer, int sample_count);
    void st_fill_planar(St_Sampler* sampler_left, St_Sampler* sampler_right, float* buffer, int sample_count);
}
