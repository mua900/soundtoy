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

    static void bytecode_step_time(void* program, float step);
	static bool bytecode_set_expression(void* program, String expression_string);
	static float bytecode_evaluate(void* program);
	static void bytecode_fill(void* program, float* buffer, int sample_count);
	static void bytecode_fill_strided(void* program, float* buffer, int sample_count);
	static void bytecode_fill_interleaved(void* program_left, void* program_right, float* buffer, int sample_count);
	static void bytecode_fill_planar(void* program_left, void* program_right, float* buffer, int sample_count);

    struct St_Sampler {
        Bytecode_Program* program = nullptr;
    };

    // @todo thread safe
    static const char* st_last_error = nullptr;

    bool st_initialize() {
        return true;
    }

    const char* st_get_last_error() {
        return st_last_error;
    }

    St_Sampler* st_sampler_create(int sample_rate) {
        St_Sampler* sampler = new St_Sampler;

        if (!sampler) {
            st_last_error = "Could not allocate sampler";
            return nullptr;
        }

        sampler->program = new Bytecode_Program();
        st_sampler_set_sample_rate(sampler, sample_rate);

        return sampler;
    }

    St_Sampler* st_sampler_copy(St_Sampler* sampler) {
        St_Sampler* copy = new St_Sampler;

        if (!sampler) {
            st_last_error = "Could not allocate copy sampler";
            return nullptr;
        }

        copy->program = new Bytecode_Program;
        *((Bytecode_Program*)copy->program) = *((Bytecode_Program*)sampler->program);

        return copy;
    }

    void st_sampler_destroy(St_Sampler* sampler) {
        free(sampler->program);
        free(sampler);
    }

    bool st_check_expression_string(const St_Sampler* sampler_or_null, const char* expression_string, int length) {
        Parser parser;
        String expression = String(expression_string, length);

        if (sampler_or_null) {
			parser.set_symbols(sampler_or_null->program->symbols);
            return parser.check_expression_string(expression);
        }
        else {
            return parser.syntax_check(expression);
        }
    }

    bool st_sampler_set_expression(St_Sampler* sampler, const char* expression_string, int length) {
        String expression = String(expression_string, length);
        return bytecode_set_expression(sampler->program, expression);
    }

    float st_sampler_evaluate(const St_Sampler* sampler) {
        return bytecode_evaluate(sampler->program);
    }

    void st_sampler_step_time(St_Sampler* sampler, float step_size) {
        bytecode_step_time(sampler->program, step_size);
    }

    void st_sampler_set_sample_rate(St_Sampler* sampler, float sample_rate) {
        sampler->program->set_sample_rate(sample_rate);
    }

    void st_sampler_set_sample_time(St_Sampler* sampler, float sample_time) {
        sampler->program->set_sample_time(sample_time);
    }

    float st_sampler_get_sample_time(const St_Sampler* sampler) {
        return sampler->program->get_sample_time();
    }

    float st_sampler_get_sample_rate(const St_Sampler* sampler) {
        return sampler->program->get_sample_rate();
    }

    int st_sampler_register_variable(St_Sampler* sampler, const char* name, int length, Variable_Type type) {
        String symbol = String(name, length);
        Variable var = Variable(symbol, type);

        Find_Result find = find_symbol(sampler->program->symbols, symbol);
        if (find.found) {
            st_last_error = "Trying to register the same variable name more than once.";
            return find.index;
        }

        int var_id = sampler->program->add_symbol(var);
        return var_id;
    }

    bool st_sampler_set_variable_value(St_Sampler* sampler, int variable, float value) {
        if (!sampler->program->variables.in_bounds(variable)) {
            return false;
        }

        sampler->program->variables.get_ref(variable) = value;
        return true;
    }

    float st_sampler_get_variable_value(const St_Sampler* sampler, int variable) {
        return sampler->program->variables.get(variable);
    }

    const char* st_sampler_get_variable_name_at_index(const St_Sampler* sampler, int index) {
        // @fix returning raw pointer from string that is not guaranteed to be null terminated
		// if we make a copy then the caller needs to free which isn't nice
        return sampler->program->symbols.get(index).name.data;
    }

    int st_sampler_get_variable_count(const St_Sampler* sampler) {
        return sampler->program->variables.size();
    }

    void st_set_input_stream(St_Sampler* sampler, float* input_stream, int input_stream_size, int stride) {
        sampler->program->set_input_stream(InputStream(Array<float>(input_stream, input_stream_size), stride));
    }

    void st_clear_input_stream(St_Sampler* sampler) {
        sampler->program->input_stream = InputStream();
    }

    void st_fill(St_Sampler* sampler, float* buffer, int length) {
        bytecode_fill(sampler->program, buffer, length);
    }

	void st_fill_strided(St_Sampler* sampler, float* buffer, int sample_count) {
        bytecode_fill_strided(sampler->program, buffer, sample_count);
	}

    void st_fill_interleaved(St_Sampler* sampler_left, St_Sampler* sampler_right, float* buffer, int sample_count) {
        bytecode_fill_interleaved(sampler_left->program, sampler_right->program, buffer, sample_count);
    }

	void st_fill_planar(St_Sampler* sampler_left, St_Sampler* sampler_right, float* buffer, int sample_count) {
        bytecode_fill_planar(sampler_left->program, sampler_right->program, buffer, sample_count);
	}


    static void bytecode_step_time(void* evaluator, float step) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;
        program->step_time(step);
    }
    static bool bytecode_set_expression(void* evaluator, String expression_string) {
		Bytecode_Program* program = (Bytecode_Program*) evaluator;

        Parser parser = {};

		parser.set_symbols(program->symbols);
        Expr* expression = parser.parse(expression_string);
        if (!expression) {
            return false;
        }

        if (expression->flags & EXPR_USES_INPUT_SAMPLES)
        {
            if (program->input_stream.samples.data == nullptr)
            {
                st_last_error = "No input stream to get samples.";

                return false;
            }
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


    static double bc_evaluate(Bytecode_Program* program) {
        double result = bytecode_run(*program);
        result = flush_subnormal_dp(CLAMP(result, -1.0, 1.0));
        return result;
    }
}
