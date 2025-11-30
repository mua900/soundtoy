#include "api.h"

#include "common.h"
#include "evaluator.h"
#include "bytecode.h"

extern "C" {

    static Evaluator_Type active_evaluator_type = TREE_INTERPRETER;

    static Expr* sample_expression;  // @fix move this inside the evaluator
    static Evaluator evaluator;

    static Bytecode_Program bytecode_program_left;
    static Bytecode_Program bytecode_program_right;

    bool set_eval_expression(const char* expression_string, int length) {
        Parser parser;
        Array<Expr*> expression = parser.parse(String(expression_string, length));  // @fix

        Eval eval = evaluator.evaluate(expression.get_or_default(0));
        if (!eval.success) {
            return false;
        }

        bool bytecode_compile_success = bytecode_compile_expression(bytecode_program_left, expression.get_or_default(0));
        if (!bytecode_compile_success) {
            return false;
        }

        sample_expression = expression.get_or_default(0);

        return true;
    }

    bool set_eval_expression_left(const char* expression_string, int length) {
        return set_eval_expression(expression_string, length);
    }

    bool set_eval_expression_right(const char* expression_string, int length) {
        Parser parser;
        Array<Expr*> expression = parser.parse(String(expression_string, length));  // @fix
        bool compilation_success = bytecode_compile_expression(bytecode_program_right, expression.get_or_default(0));
        return compilation_success;
    }

    void set_sample_rate(double sample_rate) {
        if (active_evaluator_type == TREE_INTERPRETER) {
            evaluator.set(sample_rate, evaluator.get_time());
        }
    }

    void set_sample_time(double sample_time) {
        if (active_evaluator_type == TREE_INTERPRETER) {
            evaluator.set(evaluator.get_sample_rate(), sample_time);
        }
    }

    double get_active_sample_rate() {
        if (active_evaluator_type == TREE_INTERPRETER) {
            return evaluator.get_sample_rate();
        }

        return 0.0;
    }
    double get_active_sample_time() {
        if (active_evaluator_type == TREE_INTERPRETER) {
            return evaluator.get_time();
        }

        return 0.0;
    }

    void set_active_evaluator(Evaluator_Type type) {
        active_evaluator_type = type;
    }

    float get_sample() {
        if (active_evaluator_type == TREE_INTERPRETER)
        {
            double inv_sample_rate = 1.0 / evaluator.get_sample_rate();
            Eval sample = evaluator.evaluate(sample_expression);
            evaluator.step_time(inv_sample_rate);
            return sample.value;
        }
        else if (active_evaluator_type == BYTECODE_INTERPRETER)
        {
            double inv_sample_rate = 1.0 / bytecode_program_left.get_sample_rate();
            float sample = bytecode_run(bytecode_program_left);
            bytecode_program_left.step_time(inv_sample_rate);
            return sample;
        }
        else {
            return 0.0;
        }
    }

    // length in elements
    void fill_sample_buffer(float* buffer, int length) {
        if (active_evaluator_type == TREE_INTERPRETER)
        {
            double inv_sample_rate = 1.0 / evaluator.get_sample_rate();
            for (int i = 0; i < length; i++)
            {
                Eval sample = evaluator.evaluate(sample_expression);
                evaluator.step_time(inv_sample_rate);

                buffer[i] = sample.value;
            }
        }
        else if (active_evaluator_type == BYTECODE_INTERPRETER)
        {
            double inv_sample_rate = 1.0 / bytecode_program_left.get_sample_rate();
            for (int i = 0; i < length; i++)
            {
                float sample = bytecode_run(bytecode_program_left);
                bytecode_program_left.step_time(inv_sample_rate);

                buffer[i] = sample;
            }
        }
    }

}