#pragma once

extern "C" {

    enum Evaluator_Type : int {
        BYTECODE_INTERPRETER = 0,
        TREE_INTERPRETER = 1,
    };

    bool set_eval_expression(const char* expression_string, int length);
    bool set_eval_expression_left(const char* expression_string, int length);
    bool set_eval_expression_right(const char* expression_string, int length);

    void set_sample_rate(double sample_rate);
    void set_sample_time(double sample_time);

    double get_active_sample_rate();
    double get_active_sample_time();

    void set_active_evaluator(Evaluator_Type type);

    float get_sample();
    // length in elements
    void fill_sample_buffer(float* buffer, int length);
}