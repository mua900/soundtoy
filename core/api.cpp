#include "api.h"

#include "common.h"
#include "template.h"
#include "evaluator.h"
#include "bytecode.h"

extern "C" {

    typedef Array<String> (*evaluator_symbol_table_get_f)(void* evaluator);
    typedef bool (*evaluator_set_expression_f)(void* evaluator, String expression);
    typedef float (*evaluate_expression_f)(void* evaluator);
    typedef void (*evaluator_fill_buffer_f)(void* evaluator, float* buffer, int count);
    typedef void (*evaluator_fill_buffer_interleaved_f)(void* evaluator_0, void* evaluator_1, float* buffer, int count);

    struct St_Sampler {
        Evaluator_Type evaluator_type = Evaluator_Type::TREE_INTERP;
        void* evaluator = nullptr;
        evaluate_expression_f evaluate_expression = nullptr;
        evaluator_symbol_table_get_f evaluator_symbol_table_get = nullptr;
        evaluator_set_expression_f evaluator_set_expression = nullptr;
        evaluator_fill_buffer_f evaluator_fill_buffer = nullptr;
        evaluator_fill_buffer_interleaved_f evaluator_fill_buffer_interleaved = nullptr;
    };

    // @todo thread safe
    static const char* st_last_error = NULL;

    bool st_initialize() {
        return true;
    }

    const char* get_last_error() {
        return st_last_error;
    }

    St_Sampler* st_sampler_create(Evaluator_Type evaluator_type, int sample_rate) {
        St_Sampler* sampler = (St_Sampler*) malloc(sizeof(St_Sampler));

        if (!sampler) {
            return NULL;
        }

        sampler->evaluator_type = evaluator_type;

        if (evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            sampler->evaluator = malloc(sizeof(Bytecode_Program));
        }
        else if (evaluator_type == Evaluator_Type::TREE_INTERP) {
            sampler->evaluator = malloc(sizeof(Tree_Evaluator));
        }

        return sampler;
    }

    void st_sampler_destroy(St_Sampler* sampler) {
        free(sampler->evaluator);
        free(sampler);
    }

    bool st_check_expression_string(const St_Sampler* sampler_or_null, const char* expression_string, int length) {
        Parser parser;
        String expression = String(expression_string, length);

        if (sampler_or_null) {
            return parser.check_expression_string(expression, sampler_or_null->evaluator_symbol_table_get(sampler_or_null->evaluator));
        }
        else {
            return parser.syntax_check(expression);
        }
    }

    bool st_sampler_set_expression(St_Sampler* sampler, const char* expression_string, int length) {
        String expression = String(expression_string, length);
        return sampler->evaluator_set_expression(sampler->evaluator, expression);
    }

    float st_sampler_evaluate(const St_Sampler* sampler) {
        return sampler->evaluate_expression(sampler->evaluator);
    }

    void st_sampler_set_sample_rate(St_Sampler* sampler, float sample_rate) {
        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;
            program->set_sample_rate(sample_rate);
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;
            tree_interp->set(sample_rate, tree_interp->get_time());
        }
    }

    void st_sampler_set_sample_time(St_Sampler* sampler, float sample_time) {
        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;
            program->set_sample_time(sample_time);
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;
            tree_interp->set(tree_interp->get_sample_rate(), sample_time);
        }
    }

    float st_sampler_get_sample_time(const St_Sampler* sampler) {
        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;
            return program->get_sample_time();
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;
            return tree_interp->get_time();
        }
        else {
            st_last_error = "Invalid evaluator type";
            return 0.0;
        }
    }

    float st_sampler_get_sample_rate(const St_Sampler* sampler) {
        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;
            return program->get_sample_time();
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;
            return tree_interp->get_time();
        }
        else {
            st_last_error = "Invalid evaluator type";
            return 0.0;
        }
    }

    int st_sampler_register_variable(St_Sampler* sampler, const char* name, int length) {
        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;

            String symbol = String();
            Find_Result find = program->symbols.find(symbol);
            if (find.found) {
                st_last_error = "Trying to register the same variable name more than once.";
                return find.index;
            }

            int var_id = program->add_symbol(symbol);
            return var_id;
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;

            String symbol = String();
            Find_Result find = tree_interp->symbols.find(symbol);
            if (find.found) {
                st_last_error = "Trying to register the same variable name more than once.";
                return find.index;
            }

            int var_id = tree_interp->add_symbol(symbol);
            return var_id;
        }
        else {
            st_last_error = "Invalid evaluator type";
            return -1;
        }
    }

    bool st_sampler_set_variable_value(St_Sampler* sampler, int variable, float value) {
        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;
            if (!program->variables.in_bounds(variable)) {
                return false;
            }

            program->variables.get_ref(variable) = value;
            return true;
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;
            if (!tree_interp->variables.in_bounds(variable)) {
                return false;
            }

            tree_interp->variables.get_ref(variable) = value;
            return true;
        }
        else {
            st_last_error = "Invalid evaluator type";
            return false;
        }
    }

    float st_sampler_get_variable_value(const St_Sampler* sampler, int variable) {
        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;
            return program->variables.get(variable);
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;
            return tree_interp->variables.get(variable);
        }
        else {
            st_last_error = "Invalid evaluator type";
            return 0.0;
        }
    }

    const char* st_sampler_get_variable_name_at_index(const St_Sampler* sampler, int index) {
        // @fix returning raw pointer from string
        // although it is coming from a user supplied C string and it probably is null terminated.
        return sampler->evaluator_symbol_table_get(sampler->evaluator).get(index).data;
    }

    int st_sampler_get_variable_count(const St_Sampler* sampler) {
        return sampler->evaluator_symbol_table_get(sampler->evaluator).size;
    }

    void st_fill(St_Sampler* sampler, float* buffer, int length) {
        sampler->evaluator_fill_buffer(sampler->evaluator, buffer, length);
    }

    static Array<String> bytecode_symbol_table_get(void* evaluator) {
        Bytecode_Program* program = (Bytecode_Program*) evaluator;
        return program->symbols;
    }
    static bool bytecode_set_expression(void* evaluator, String expression_string) {
        Bytecode_Program* program = (Bytecode_Program*) evaluator;

        Parser parser = {};

        Expr* expression = parser.parse(expression_string);
        if (!expression) {
            return false;
        }

        bool compilation_success = bytecode_compile_expression(*program, expression);
        if (!compilation_success) {
            return false;
        }

        return true;
    }
    static float bytecode_evaluate(void* evaluator) {
        Bytecode_Program* program = (Bytecode_Program*) evaluator;
        return bytecode_run(*program);
    }
    static void bytecode_fill_buffer(void* evaluator, float* buffer, int count) {
        Bytecode_Program* program = (Bytecode_Program*) evaluator;

        for (int i = 0; i < count; i++) {
            buffer[i] = bytecode_run(*program);
        }
    }
    static void bytecode_fill_buffer_interleaved(void* evaluator_0, void* evaluator_1, float* buffer, int count) {
        Bytecode_Program* program_left = (Bytecode_Program*) evaluator_0;
        Bytecode_Program* program_right = (Bytecode_Program*) evaluator_1;

        for (int i = 0; i < count; i++) {
            int left = i * 2 + 0;
            int right = i * 2 + 1;

            buffer[left] = bytecode_run(*program_left);
            buffer[right] = bytecode_run(*program_right);
        }
    }

    static Array<String> tree_interp_symbol_table_get(void* evaluator) {
        Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;
        return tree_interp->symbols;
    }
    static bool tree_interp_set_expression(void* evaluator, String expression_string) {
        Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;

        Parser parser = {};

        Expr* expression = parser.parse(expression_string);
        if (!expression) {
            return false;
        }

        auto eval = tree_interp->evaluate_expression(expression);
        if (!eval.success) {
            return false;
        }

        tree_interp->expression = expression;
        return true;
    }
    static float tree_interp_evaluate(void* evaluator) {
        Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;

        return tree_interp->evaluate().value;
    }
    static void tree_interp_fill_buffer(void* evaluator, float* buffer, int count) {
        Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;

        for (int i = 0; i < count; i++) {
            buffer[i] = tree_interp->evaluate().value;
        }
    }
    static void tree_interp_fill_buffer_interleaved(void* evaluator_0, void* evaluator_1, float* buffer, int count) {
        Tree_Evaluator* tree_left = (Tree_Evaluator*) evaluator_0;
        Tree_Evaluator* tree_right = (Tree_Evaluator*) evaluator_1;

        for (int i = 0; i < count; i++) {
            int left = i * 2 + 0;
            int right = i * 2 + 1;

            buffer[left] = tree_left->evaluate().value;
            buffer[right] = tree_right->evaluate().value;
        }
    }

}