#include "api.h"

#include "common.h"
#include "template.h"
#include "evaluator.h"
#include "bytecode.h"
#include "platform.h"

#include <bit>
#include <cmath>

extern "C" {
    static const double min_flt_dp = std::bit_cast<double>(0x0010000000000000);
    static const float  min_flt_sp = std::bit_cast<float>(0x00800000);

    double flush_subnormal_dp(double x) {
        return (fabs(x) < min_flt_dp) ? 0.0 : x;
    }

    float flush_subnormal_sp(float x) {
        return (fabsf(x) < min_flt_sp) ? 0.0 : x;
    }

    static double bc_evaluate(Bytecode_Program* program);
    static double tree_evaluate(Tree_Evaluator* tree_interp);

    static void bytecode_step_time(void* program, float step);
	static Array<Variable> bytecode_symbol_table_get(void* program);
	static bool bytecode_set_expression(void* program, String expression_string);
	static float bytecode_evaluate(void* program);
	static void bytecode_fill(void* program, float* buffer, int sample_count);
	static void bytecode_fill_strided(void* program, float* buffer, int sample_count);
	static void bytecode_fill_interleaved(void* program_left, void* program_right, float* buffer, int sample_count);
	static void bytecode_fill_planar(void* program_left, void* program_right, float* buffer, int sample_count);

    static void tree_interp_step_time(void* evaluator, float step);
	static Array<Variable> tree_interp_symbol_table_get(void* evaluator);
	static bool tree_interp_set_expression(void* evaluator, String expression_string);
	static float tree_interp_evaluate(void* evaluator);
	static void tree_interp_fill(void* evaluator, float* buffer, int sample_count);
	static void tree_interp_fill_strided(void* evaluator_0, float* buffer, int sample_count);
	static void tree_interp_fill_interleaved(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count);
	static void tree_interp_fill_planar(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count);


    typedef Array<Variable> (*evaluator_symbol_table_get_f)(void* evaluator);
    typedef bool (*evaluator_set_expression_f)(void* evaluator, String expression);
    typedef float (*evaluate_expression_f)(void* evaluator);
    typedef void (*evaluator_step_time_f)(void* evaluator, float step);	
    typedef void (*evaluator_fill_f)(void* evaluator, float* buffer, int sample_count);
	typedef void (*evaluator_fill_strided_f)(void* evaluator, float* buffer, int sample_count);
	// both evaluators should be of the same type
    typedef void (*evaluator_fill_interleaved_f)(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count);
    typedef void (*evaluator_fill_planar_f)(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count);

    struct Function_Table {
        evaluate_expression_f evaluate_expression = nullptr;
        evaluator_symbol_table_get_f evaluator_symbol_table_get = nullptr;
        evaluator_set_expression_f evaluator_set_expression = nullptr;
        evaluator_step_time_f evaluator_step_time = nullptr;
        evaluator_fill_f evaluator_fill = nullptr;
        evaluator_fill_strided_f evaluator_fill_strided = nullptr;
    	evaluator_fill_interleaved_f evaluator_fill_interleaved = nullptr;
		evaluator_fill_planar_f evaluator_fill_planar = nullptr;
    };

    struct St_Sampler {
        Evaluator_Type evaluator_type = Evaluator_Type::TREE_INTERP;
        void* evaluator = nullptr;
        Function_Table table = {};
    };

    // @todo thread safe
    static const char* st_last_error = nullptr;
    static bool allow_subnormals = false;

    bool st_initialize(bool p_allow_subnormals) {
        allow_subnormals = p_allow_subnormals;
        return true;
    }

    const char* st_get_last_error() {
        return st_last_error;
    }

    St_Sampler* st_sampler_create(Evaluator_Type evaluator_type, int sample_rate) {
        St_Sampler* sampler = new St_Sampler;

        if (!sampler) {
            st_last_error = "Could not allocate sampler";
            return nullptr;
        }

        sampler->evaluator_type = evaluator_type;

        if (evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            sampler->evaluator = new Bytecode_Program();

            // @update
            sampler->table.evaluate_expression = bytecode_evaluate;
            sampler->table.evaluator_symbol_table_get = bytecode_symbol_table_get;
            sampler->table.evaluator_set_expression = bytecode_set_expression;
            sampler->table.evaluator_fill = bytecode_fill;
            sampler->table.evaluator_fill_strided = bytecode_fill_strided;
			sampler->table.evaluator_fill_interleaved = bytecode_fill_interleaved;
			sampler->table.evaluator_fill_planar = bytecode_fill_planar;
            sampler->table.evaluator_step_time = bytecode_step_time;
        }
        else if (evaluator_type == Evaluator_Type::TREE_INTERP) {
            sampler->evaluator = new Tree_Evaluator();

			// @update
            sampler->table.evaluate_expression = tree_interp_evaluate;
            sampler->table.evaluator_symbol_table_get = tree_interp_symbol_table_get;
            sampler->table.evaluator_set_expression = tree_interp_set_expression;
            sampler->table.evaluator_step_time = tree_interp_step_time;
            sampler->table.evaluator_fill = tree_interp_fill;
            sampler->table.evaluator_fill_strided = tree_interp_fill_strided;
			sampler->table.evaluator_fill_interleaved = tree_interp_fill_interleaved;
			sampler->table.evaluator_fill_planar = tree_interp_fill_planar;
        }

        st_sampler_set_sample_rate(sampler, sample_rate);

        return sampler;
    }

    St_Sampler* st_sampler_copy(St_Sampler* sampler) {
        St_Sampler* copy = new St_Sampler;

        if (!sampler) {
            st_last_error = "Could not allocate copy sampler";
            return nullptr;
        }

        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            copy->evaluator = new Bytecode_Program;
            *((Bytecode_Program*)copy->evaluator) = *((Bytecode_Program*)sampler->evaluator);
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            copy->evaluator = new Tree_Evaluator;
            *((Tree_Evaluator*)copy->evaluator) = *((Tree_Evaluator*)sampler->evaluator);
        }
        else {
            st_last_error = "Invalid sampler type";
            return nullptr;
        }

        copy->evaluator_type = sampler->evaluator_type;
        copy->table = sampler->table;

        return copy;
    }

    void st_sampler_destroy(St_Sampler* sampler) {
        free(sampler->evaluator);
        free(sampler);
    }

    bool st_check_expression_string(const St_Sampler* sampler_or_null, const char* expression_string, int length) {
        Parser parser;
        String expression = String(expression_string, length);

        if (sampler_or_null) {
			parser.set_symbols(sampler_or_null->table.evaluator_symbol_table_get(sampler_or_null->evaluator));
            return parser.check_expression_string(expression);
        }
        else {
            return parser.syntax_check(expression);
        }
    }

    bool st_sampler_set_expression(St_Sampler* sampler, const char* expression_string, int length) {
        String expression = String(expression_string, length);
        return sampler->table.evaluator_set_expression(sampler->evaluator, expression);
    }

    float st_sampler_evaluate(const St_Sampler* sampler) {
        return sampler->table.evaluate_expression(sampler->evaluator);
    }

    void st_sampler_step_time(St_Sampler* sampler, float step_size) {
        sampler->table.evaluator_step_time(sampler->evaluator, step_size);
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

    int st_sampler_register_variable(St_Sampler* sampler, const char* name, int length, Variable_Type type) {
        String symbol = String(name, length);
        Variable var = Variable(symbol, type);

        if (sampler->evaluator_type == Evaluator_Type::BYTECODE_INTERP) {
            auto program = (Bytecode_Program*) sampler->evaluator;

            Find_Result find = find_symbol(program->symbols, symbol);
            if (find.found) {
                st_last_error = "Trying to register the same variable name more than once.";
                return find.index;
            }

            int var_id = program->add_symbol(var);
            return var_id;
        }
        else if (sampler->evaluator_type == Evaluator_Type::TREE_INTERP) {
            auto tree_interp = (Tree_Evaluator*) sampler->evaluator;

            Find_Result find = find_symbol(tree_interp->symbols,symbol);
            if (find.found) {
                st_last_error = "Trying to register the same variable name more than once.";
                return find.index;
            }

            int var_id = tree_interp->add_symbol(var);
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
        return sampler->table.evaluator_symbol_table_get(sampler->evaluator).get(index).name.data;
    }

    int st_sampler_get_variable_count(const St_Sampler* sampler) {
        return sampler->table.evaluator_symbol_table_get(sampler->evaluator).size;
    }

    void st_fill(St_Sampler* sampler, float* buffer, int length) {
        sampler->table.evaluator_fill(sampler->evaluator, buffer, length);
    }

	void st_fill_strided(St_Sampler* sampler, float* buffer, int sample_count) {
		sampler->table.evaluator_fill_strided(sampler->evaluator, buffer, sample_count);
	}

    void st_fill_interleaved(St_Sampler* sampler_left, St_Sampler* sampler_right, float* buffer, int sample_count) {
        if (sampler_left->evaluator_type == sampler_right->evaluator_type) {
			ASSERT(sampler_left->table.evaluator_fill_interleaved == sampler_right->table.evaluator_fill_interleaved);

            sampler_left->table.evaluator_fill_interleaved(sampler_left->evaluator, sampler_right->evaluator, buffer, sample_count);
        }
        else {
			sampler_left->table.evaluator_fill_strided(sampler_left->evaluator, buffer + 0, sample_count);
			sampler_right->table.evaluator_fill_strided(sampler_right->evaluator, buffer + 1, sample_count);
		}
    }

	void st_fill_planar(St_Sampler* sampler_left, St_Sampler* sampler_right, float* buffer, int sample_count) {
		// @todo the else block is enough
        if (sampler_left->evaluator_type == sampler_right->evaluator_type) {
			ASSERT(sampler_left->table.evaluator_fill_planar == sampler_right->table.evaluator_fill_planar);

            sampler_left->table.evaluator_fill_planar(sampler_left->evaluator, sampler_right->evaluator, buffer, sample_count);
        }
        else {
			sampler_left->table.evaluator_fill(sampler_left->evaluator, buffer + 0,              sample_count / 2);
			sampler_right->table.evaluator_fill(sampler_right->evaluator, buffer + sample_count, sample_count / 2);
		}
	}


    static void bytecode_step_time(void* evaluator, float step) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;
        program->step_time(step);
    }
    static Array<Variable> bytecode_symbol_table_get(void* evaluator) {
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
        return bc_evaluate(program);
    }
    static void bytecode_fill(void* evaluator, float* buffer, int count) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;

        double inv_sample_rate = 1.0 / program->get_sample_rate();

        for (int i = 0; i < count; i++) {
            buffer[i] = bc_evaluate(program);
	        program->step_time(inv_sample_rate);
        }
    }
	static void bytecode_fill_strided(void* evaluator, float* buffer, int sample_count) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;

		double inv_sample_rate = 1.0 / program->get_sample_rate();
		for (int i = 0; i < sample_count; i++) {
			int index = i * 2;
			buffer[index] = bc_evaluate(program);

			program->step_time(inv_sample_rate);
		}
	}
    static void bytecode_fill_interleaved(void* evaluator_left, void* evaluator_right, float* buffer, int count) {
		Bytecode_Program* program_left = (Bytecode_Program*) evaluator_left;
		Bytecode_Program* program_right = (Bytecode_Program*) evaluator_right;
		
    	double left_inv_sr = 1.0 / program_left->get_sample_rate();
    	double right_inv_sr = 1.0 / program_right->get_sample_rate();

        for (int i = 0; i < count; i++) {
            int left = i * 2 + 0;
            int right = i * 2 + 1;

            buffer[left] = bc_evaluate(program_left);
            buffer[right] = bc_evaluate(program_right);

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
    		buffer[i] = bc_evaluate(program_left);
    		program_left->step_time(left_inv_sr);
    	}
    	for (int i = sample_count / 2; i < sample_count; i++) {
    		buffer[i] = bc_evaluate(program_right);
    		program_right->step_time(right_inv_sr);
    	}
    }


    static void tree_interp_step_time(void* evaluator, float step) {
		Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;
        tree_interp->step_time(step);
    }
    static Array<Variable> tree_interp_symbol_table_get(void* evaluator) {
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

        // reset state
        tree_interp->builtins[BUILTIN_VARIABLE_TIME] = 0.0;

        auto eval = tree_interp->evaluate_expression(expression);
        if (!eval.success) {
            return false;
        }

        tree_interp->expression = expression;
        return true;
    }
    static float tree_interp_evaluate(void* evaluator) {
        Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;
        return tree_evaluate(tree_interp);
    }
    static void tree_interp_fill(void* evaluator, float* buffer, int count) {
		Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;

	    double inv_sample_rate = 1.0 / tree_interp->get_sample_rate();

        for (int i = 0; i < count; i++) {
            buffer[i] = tree_evaluate(tree_interp);
	        tree_interp->step_time(inv_sample_rate);
        }
    }
	static void tree_interp_fill_strided(void* evaluator, float* buffer, int sample_count) {
		Tree_Evaluator* tree_interp = (Tree_Evaluator*) evaluator;
		
		double inv_sample_rate = tree_interp->get_sample_rate();
		for (int i = 0; i < sample_count; i++) {
			int index = i * 2;
			buffer[index] = tree_evaluate(tree_interp);
			tree_interp->step_time(inv_sample_rate);
		}
	}
    static void tree_interp_fill_interleaved(void* evaluator_left, void* evaluator_right, float* buffer, int sample_count) {
		Tree_Evaluator* tree_left = (Tree_Evaluator*) evaluator_left;
		Tree_Evaluator* tree_right = (Tree_Evaluator*) evaluator_right;
		
        double left_inv_sr = 1.0 / tree_left->get_sample_rate();
        double right_inv_sr = 1.0 / tree_right->get_sample_rate();

        for (int i = 0; i < sample_count; i++) {
            int left = i * 2 + 0;
            int right = i * 2 + 1;

            buffer[left] = tree_evaluate(tree_left);
            buffer[right] = tree_evaluate(tree_right);

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
			buffer[i] = tree_evaluate(tree_left);
			tree_left->step_time(left_inv_sr);
		}
		for (int i = sample_count / 2; i < sample_count; i++) {
			buffer[i] = tree_evaluate(tree_right);
			tree_right->step_time(right_inv_sr);
		}
	}


    static double bc_evaluate(Bytecode_Program* program) {
        double result = bytecode_run(*program);
        result = flush_subnormal_dp(CLAMP(result, -1.0, 1.0));
        return result;
    }

    static double tree_evaluate(Tree_Evaluator* tree_interp) {
        double result = tree_interp->evaluate().value;
        result = flush_subnormal_dp(CLAMP(result, -1.0, 1.0));
        return result;
    }
}
