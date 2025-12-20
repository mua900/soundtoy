#include "api.h"

#include "common.h"
#include "template.h"
#include "evaluator.h"
#include "bytecode.h"

extern "C" {

    static void bytecode_step_time(void* program, float step);
	static Array<String> bytecode_symbol_table_get(void* program);
	static bool bytecode_set_expression(void* program, String expression_string);
	static float bytecode_evaluate(void* program);
	static void bytecode_fill_buffer(void* program, float* buffer, int sample_count);
	static void bytecode_fill_buffer_interleaved(void* program, float* buffer, int sample_count);
	static void bytecode_fill_buffer_interleaved_double(void* program_left, void* program_right, float* buffer, int sample_count);
	static void bytecode_fill_planar(void* program_left, void* program_right, float* buffer, int sample_count);

    static void tree_interp_step_time(void* evaluator, float step);
	static Array<String> tree_interp_symbol_table_get(void* evaluator);
	static bool tree_interp_set_expression(void* evaluator, String expression_string);
	static float tree_interp_evaluate(void* evaluator);
	static void tree_interp_fill_buffer(void* evaluator, float* buffer, int sample_count);
	static void tree_interp_fill_buffer_interleaved(void* evaluator_0, float* buffer, int sample_count);
	static void tree_interp_fill_buffer_interleaved_double(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count);
	static void tree_interp_fill_planar(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count);


    typedef Array<String> (*evaluator_symbol_table_get_f)(void* evaluator);
    typedef bool (*evaluator_set_expression_f)(void* evaluator, String expression);
    typedef float (*evaluate_expression_f)(void* evaluator);
    typedef void (*evaluator_fill_buffer_f)(void* evaluator, float* buffer, int sample_count);
	typedef void (*evaluator_fill_buffer_interleaved_f)(void* evaluator, float* buffer, int sample_count);
    typedef void (*evaluator_step_time_f)(void* evaluator, float step);	
    // both evaluators should be of the same type
    typedef void (*evaluator_fill_buffer_interleaved_double_f)(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count);
    typedef void (*evaluator_fill_planar)(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count);

    struct St_Sampler {
        Evaluator_Type evaluator_type = Evaluator_Type::TREE_INTERP;
        void* evaluator = nullptr;
        evaluate_expression_f evaluate_expression = nullptr;
        evaluator_symbol_table_get_f evaluator_symbol_table_get = nullptr;
        evaluator_set_expression_f evaluator_set_expression = nullptr;
        evaluator_fill_buffer_f evaluator_fill_buffer = nullptr;
        evaluator_fill_buffer_interleaved_f evaluator_fill_buffer_interleaved = nullptr;
    	evaluator_fill_buffer_interleaved_double_f evaluator_fill_buffer_interleaved_double = nullptr;
        evaluator_step_time_f evaluator_step_time = nullptr;
    };

    // @todo thread safe
    static const char* st_last_error = nullptr;

    bool st_initialize() {
        return true;
    }

    const char* st_get_last_error() {
        return st_last_error;
    }

    St_Sampler* st_sampler_create(Evaluator_Type evaluator_type, int sample_rate) {
        St_Sampler* sampler = (St_Sampler*) malloc(sizeof(St_Sampler));

        if (!sampler) {
            st_last_error = "Could not allocate sampler";
            return nullptr;
        }

        sampler->evaluator_type = evaluator_type;

        if (evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            sampler->evaluator = new Bytecode_Program();

            // @update
            sampler->evaluate_expression = bytecode_evaluate;
            sampler->evaluator_symbol_table_get = bytecode_symbol_table_get;
            sampler->evaluator_set_expression = bytecode_set_expression;
            sampler->evaluator_fill_buffer = bytecode_fill_buffer;
            sampler->evaluator_fill_buffer_interleaved = bytecode_fill_buffer_interleaved;
			sampler->evaluator_fill_buffer_interleaved_double = bytecode_fill_buffer_interleaved_double;
            sampler->evaluator_step_time = bytecode_step_time;
        }
        else if (evaluator_type == Evaluator_Type::TREE_INTERP) {
            sampler->evaluator = new Tree_Evaluator();

			// @update
            sampler->evaluate_expression = tree_interp_evaluate;
            sampler->evaluator_symbol_table_get = tree_interp_symbol_table_get;
            sampler->evaluator_set_expression = tree_interp_set_expression;
            sampler->evaluator_fill_buffer = tree_interp_fill_buffer;
            sampler->evaluator_fill_buffer_interleaved = tree_interp_fill_buffer_interleaved;
			sampler->evaluator_fill_buffer_interleaved_double = tree_interp_fill_buffer_interleaved_double;
            sampler->evaluator_step_time = tree_interp_step_time;
        }

        st_sampler_set_sample_rate(sampler, sample_rate);

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
			parser.set_symbols(sampler_or_null->evaluator_symbol_table_get(sampler_or_null->evaluator));
            return parser.check_expression_string(expression);
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

    void st_sampler_step_time(St_Sampler* sampler, float step_size) {
        sampler->evaluator_step_time(sampler->evaluator, step_size);
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
            return program->get_sample_rate();
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;
            return tree_interp->get_sample_rate();
        }
        else {
            st_last_error = "Invalid evaluator type";
            return 0.0;
        }
    }

    int st_sampler_register_variable(St_Sampler* sampler, const char* name, int length) {
        String symbol = String(name, length);

        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;

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
        // @fix returning raw pointer from string that is not guaranteed to be null terminated
		// if we make a copy then the caller needs to free which isn't nice
        return sampler->evaluator_symbol_table_get(sampler->evaluator).get(index).data;
    }

    int st_sampler_get_variable_count(const St_Sampler* sampler) {
        return sampler->evaluator_symbol_table_get(sampler->evaluator).size;
    }

    void st_fill(St_Sampler* sampler, float* buffer, int length) {
        sampler->evaluator_fill_buffer(sampler->evaluator, buffer, length);
    }

	void st_fill_interleaved(St_Sampler* sampler, float* buffer, int sample_count) {
		sampler->evaluator_fill_buffer_interleaved(sampler->evaluator, buffer, sample_count);
	}

    void st_fill_interleaved_double(St_Sampler* sampler_0, St_Sampler* sampler_1, float* buffer, int sample_count) {
        if (sampler_0->evaluator_type == sampler_1->evaluator_type) {
			ASSERT(sampler_0->evaluator_fill_buffer_interleaved_double == sampler_1->evaluator_fill_buffer_interleaved_double);

            sampler_0->evaluator_fill_buffer_interleaved_double(sampler_0->evaluator, sampler_1->evaluator, buffer, sample_count);
        }
        else {
			sampler_0->evaluator_fill_buffer_interleaved(sampler_0->evaluator, buffer + 0, sample_count);
			sampler_1->evaluator_fill_buffer_interleaved(sampler_1->evaluator, buffer + 1, sample_count);
		}
    }


    static void bytecode_step_time(void* evaluator, float step) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;
        program->step_time(step);
    }
    static Array<String> bytecode_symbol_table_get(void* evaluator) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;
        return program->symbols;
    }
    static bool bytecode_set_expression(void* evaluator, String expression_string) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;

        Parser parser = {};

		parser.set_symbols(bytecode_symbol_table_get(evaluator));
        Expr* expression = parser.parse(expression_string);
        if (!expression) {
            return false;
        }

		print_expression(expression);
		
        bool compilation_success = bytecode_compile_expression(*program, expression);
        if (!compilation_success) {
            return false;
        }

        return true;
    }
    static float bytecode_evaluate(void* evaluator) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;
		float result = bytecode_run(*program);
        return CLAMP(result, -1.0, 1.0);
    }
    static void bytecode_fill_buffer(void* evaluator, float* buffer, int count) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;

        double inv_sample_rate = 1.0 / program->get_sample_rate();

        for (int i = 0; i < count; i++) {
            buffer[i] = CLAMP(bytecode_run(*program), -1.0, 1.0);
	        program->step_time(inv_sample_rate);
        }
    }
	static void bytecode_fill_buffer_interleaved(void* evaluator, float* buffer, int sample_count) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;

		double inv_sample_rate = 1.0 / program->get_sample_rate();
		for (int i = 0; i < sample_count; i++) {
			int index = i * 2;
			buffer[index] = CLAMP(bytecode_run(*program), -1.0, 1.0);

			program->step_time(inv_sample_rate);
		}
	}
    static void bytecode_fill_buffer_interleaved_double(void* evaluator_left, void* evaluator_right, float* buffer, int count) {
		Bytecode_Program* program_left = (Bytecode_Program*) evaluator_left;
		Bytecode_Program* program_right = (Bytecode_Program*) evaluator_right;
		
    	double left_inv_sr = 1.0 / program_left->get_sample_rate();
    	double right_inv_sr = 1.0 / program_right->get_sample_rate();

        for (int i = 0; i < count; i++) {
            int left = i * 2 + 0;
            int right = i * 2 + 1;

            buffer[left] = CLAMP(bytecode_run(*program_left), -1.0, 1.0);
            buffer[right] = CLAMP(bytecode_run(*program_right), -1.0, 1.0);

	        program_left->step_time(left_inv_sr);
	        program_right->step_time(right_inv_sr);
        }
    }

    static void bytecode_fill_planar(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count) {
    	Bytecode_Program* program_left = (Bytecode_Program*) evaluator_left;
    	Bytecode_Program* program_right = (Bytecode_Program*) evaluator_right;

        double left_inv_sr = 1.0 / program_left->get_sample_rate();
        double right_inv_sr = 1.0 / program_right->get_sample_rate();

        if (sample_count % 2 == 1) {
        	st_last_error = "Sample count for planar stereo buffer not a multiple of 2.";
        }

    	for (int i = 0; i < sample_count / 2; i++) {
    		buffer[i] = CLAMP(bytecode_run(*program_left), -1.0, 1.0);
    		program_left->step_time(left_inv_sr);
    	}
    	for (int i = sample_count / 2; i < sample_count; i++) {
    		buffer[i] = CLAMP(bytecode_run(*program_right), -1.0, 1.0);
    		program_right->step_time(right_inv_sr);
    	}
    }


    static void tree_interp_step_time(void* evaluator, float step) {
		Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;
        tree_interp->step_time(step);
    }
    static Array<String> tree_interp_symbol_table_get(void* evaluator) {
        Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;

        return tree_interp->symbols;
    }
    static bool tree_interp_set_expression(void* evaluator, String expression_string) {
        Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;

        Parser parser = {};

		parser.set_symbols(tree_interp_symbol_table_get(evaluator));
        Expr* expression = parser.parse(expression_string);
        if (!expression) {
            return false;
        }

		print_expression(expression);
		
        auto eval = tree_interp->evaluate_expression(expression);
        if (!eval.success) {
            return false;
        }

        tree_interp->expression = expression;
        return true;
    }
    static float tree_interp_evaluate(void* evaluator) {
        Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;

        return CLAMP(tree_interp->evaluate().value, -1.0, 1.0);
    }
    static void tree_interp_fill_buffer(void* evaluator, float* buffer, int count) {
		Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;

	    double inv_sample_rate = 1.0 / tree_interp->get_sample_rate();

        for (int i = 0; i < count; i++) {
	        /*
	        we assume the evaluator is able to evaluate the expression here since
	        we are in the callback and we shouldn't be here if we have an invalid expression.
	        And we don't want to check in the callback.
	        */

            buffer[i] = CLAMP(tree_interp->evaluate().value, -1.0, 1.0);
	        tree_interp->step_time(inv_sample_rate);
        }
    }
	static void tree_interp_fill_buffer_interleaved(void* evaluator, float* buffer, int sample_count) {
		Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;
		
		double inv_sample_rate = tree_interp->get_sample_rate();
		for (int i = 0; i < sample_count; i++) {
			int index = i * 2;
			buffer[index] = tree_interp->evaluate().value;
			tree_interp->step_time(inv_sample_rate);
		}
	}
    static void tree_interp_fill_buffer_interleaved_double(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count) {
		Tree_Evaluator* tree_left = (Tree_Evaluator*) evaluator_left;
		Tree_Evaluator* tree_right = (Tree_Evaluator*) evaluator_right;
		
        double left_inv_sr = 1.0 / tree_left->get_sample_rate();
        double right_inv_sr = 1.0 / tree_right->get_sample_rate();

        for (int i = 0; i < sample_count; i++) {
            int left = i * 2 + 0;
            int right = i * 2 + 1;

            buffer[left] = CLAMP(tree_left->evaluate().value, -1.0, 1.0);
            buffer[right] = CLAMP(tree_right->evaluate().value, -1.0, 1.0);

            tree_left->step_time(left_inv_sr);
            tree_right->step_time(right_inv_sr);
        }
    }

	static void tree_interp_fill_planar(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count) {
		Tree_Evaluator* tree_left = (Tree_Evaluator*) evaluator_left;
		Tree_Evaluator* tree_right = (Tree_Evaluator*) evaluator_right;

        double left_inv_sr = 1.0 / tree_left->get_sample_rate();
        double right_inv_sr = 1.0 / tree_right->get_sample_rate();

        if (sample_count % 2 == 1) {
        	st_last_error = "Sample count for planar stereo buffer not a multiple of 2.";
        }

		for (int i = 0; i < sample_count / 2; i++) {
			buffer[i] = CLAMP(tree_left->evaluate().value, -1.0, 1.0);
			tree_left->step_time(left_inv_sr);
		}
		for (int i = sample_count / 2; i < sample_count; i++) {
			buffer[i] = CLAMP(tree_right->evaluate().value, -1.0, 1.0);
			tree_right->step_time(right_inv_sr);
		}
	}
}
