#include "api.h"

#include "common.h"
#include "template.h"
#include "evaluator.h"
#include "bytecode.h"

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

    static float bc_evaluate(Bytecode_Program* program);

	static bool bytecode_set_expression(Bytecode_Program* program, String expression_string);
	static float bytecode_evaluate(Bytecode_Program* program);
	static void bytecode_fill(Bytecode_Program* program, float* buffer, int sample_count);
	static void bytecode_fill_strided(Bytecode_Program* program, float* buffer, int sample_count);
	static void bytecode_fill_interleaved(Bytecode_Program* program_left, Bytecode_Program* program_right, float* buffer, int sample_count);
	static void bytecode_fill_planar(Bytecode_Program* program_left, Bytecode_Program* program_right, float* buffer, int sample_count);

    struct St_Sampler {
        Bytecode_Program program = {};
    };

    // @todo thread safe
    static const char* st_last_error = nullptr;

    const char* st_get_last_error() {
        return st_last_error;
    }

    St_Sampler* st_sampler_create(int sample_rate) {
        St_Sampler* sampler = new St_Sampler;

        if (!sampler) {
            st_last_error = "Could not allocate sampler";
            return nullptr;
        }

        st_sampler_set_sample_rate(sampler, sample_rate);

        return sampler;
    }

    St_Sampler* st_sampler_copy(St_Sampler* sampler) {
        St_Sampler* copy = new St_Sampler;

        if (!copy) {
            st_last_error = "Could not allocate copy sampler";
            return nullptr;
        }

        copy->program = sampler->program;

        return copy;
    }

    void st_sampler_destroy(St_Sampler* sampler) {
        delete sampler;
    }

    bool st_check_expression_string(const St_Sampler* sampler_or_null, const char* expression_string, int length) {
        Parser parser;
        String expression = String(expression_string, length);

        if (sampler_or_null) {
			parser.set_symbols(sampler_or_null->program.symbols);
            return parser.check_expression_string(expression);
        }
        else {
            return parser.syntax_check(expression);
        }
    }

    bool st_sampler_set_expression(St_Sampler* sampler, const char* expression_string, int length) {
        String expression = String(expression_string, length);
        return bytecode_set_expression(&sampler->program, expression);
    }

    float st_sampler_evaluate(St_Sampler* sampler) {
        return bytecode_evaluate(&sampler->program);
    }

    void st_sampler_step_time(St_Sampler* sampler, float step_size) {
        sampler->program.step_time(step_size);
    }

    void st_sampler_set_sample_rate(St_Sampler* sampler, float sample_rate) {
        sampler->program.set_sample_rate(sample_rate);
    }

    void st_sampler_set_sample_time(St_Sampler* sampler, float sample_time) {
        sampler->program.set_sample_time(sample_time);
    }

    float st_sampler_get_sample_time(const St_Sampler* sampler) {
        return sampler->program.get_sample_time();
    }

    float st_sampler_get_sample_rate(const St_Sampler* sampler) {
        return sampler->program.get_sample_rate();
    }

    int st_sampler_register_variable(St_Sampler* sampler, const char* name, int length, Variable_Type type) {
        String symbol = string_copy(String(name, length));
        Variable var = Variable(symbol, type);

        Find_Result find = find_symbol(sampler->program.symbols, symbol);
        if (find.found) {
            st_last_error = "Trying to register the same variable name more than once.";
            return find.index;
        }

        int var_id = sampler->program.add_symbol(var);
        return var_id;
    }

    bool st_sampler_set_variable_value(St_Sampler* sampler, int variable, float value) {
        if (!sampler->program.variables.in_bounds(variable)) {
            st_last_error = "Trying to set a variable out of bounds";
            return false;
        }

        sampler->program.variables.get_ref(variable) = value;
        return true;
    }

    float st_sampler_get_variable_value(const St_Sampler* sampler, int variable) {
        if (!sampler->program.variables.in_bounds(variable)) {
            st_last_error = "Trying to get a variable out of bounds";
            return false;
        }

        return sampler->program.variables.get(variable);
    }

    const char* st_sampler_get_variable_name_at_index(const St_Sampler* sampler, int index) {
        if (!sampler->program.variables.in_bounds(index)) {
            st_last_error = "Trying to get the name of a variable out of bounds";
            return nullptr;
        }

        // we copy the string and null terminate when we get it the first time so this is guaranteed to be null terminated
        return sampler->program.symbols.get(index).name.data;
    }

    int st_sampler_get_variable_count(const St_Sampler* sampler) {
        return sampler->program.variables.size();
    }

    void st_set_input_stream(St_Sampler* sampler, float* input_stream, int input_stream_size, int stride) {
        sampler->program.set_input_stream(InputStream(Array<float>(input_stream, input_stream_size), stride));
    }

    void st_clear_input_stream(St_Sampler* sampler) {
        sampler->program.input_stream = InputStream();
    }

    void st_fill(St_Sampler* sampler, float* buffer, int length) {
        bytecode_fill(&sampler->program, buffer, length);
    }

	void st_fill_strided(St_Sampler* sampler, float* buffer, int sample_count) {
        bytecode_fill_strided(&sampler->program, buffer, sample_count);
	}

    void st_fill_interleaved(St_Sampler* sampler_left, St_Sampler* sampler_right, float* buffer, int sample_count) {
        bytecode_fill_interleaved(&sampler_left->program, &sampler_right->program, buffer, sample_count);
    }

	void st_fill_planar(St_Sampler* sampler_left, St_Sampler* sampler_right, float* buffer, int sample_count) {
        bytecode_fill_planar(&sampler_left->program, &sampler_right->program, buffer, sample_count);
	}


    static bool bytecode_set_expression(Bytecode_Program* program, String expression_string) {
        Parser parser = {};

		parser.set_symbols(program->symbols);
        Expr* expression = parser.parse(expression_string);
        if (!expression) {
            Error error = parser.get_error();
            st_last_error = error.message;  // copy the error
            return false;
        }

        if (expression->flags & EXPR_USES_INPUT_SAMPLES)
        {
            if (program->input_stream.samples.data == nullptr)
            {
                st_last_error = "Expression uses input samples but no input stream is set.";
                free_tree(expression);
                return false;
            }
        }

		print_expression(expression);

        bytecode_compile_expression(*program, expression);

        free_tree(expression);

        return true;
    }
    static float bytecode_evaluate(Bytecode_Program* program) {
        return bc_evaluate(program);
    }
    static void bytecode_fill(Bytecode_Program* program, float* buffer, int count) {
        float inv_sample_rate = 1.0 / program->get_sample_rate();

        for (int i = 0; i < count; i++) {
            buffer[i] = bc_evaluate(program);
	        program->step_time(inv_sample_rate);
        }
    }
	static void bytecode_fill_strided(Bytecode_Program* program, float* buffer, int frame_count) {
		float inv_sample_rate = 1.0 / program->get_sample_rate();
		for (int i = 0; i < frame_count; i++) {
			int index = i * 2;
			buffer[index] = bc_evaluate(program);

			program->step_time(inv_sample_rate);
		}
	}
    static void bytecode_fill_interleaved(Bytecode_Program* program_left, Bytecode_Program* program_right, float* buffer, int count) {

    	float left_inv_sr = 1.0 / program_left->get_sample_rate();
    	float right_inv_sr = 1.0 / program_right->get_sample_rate();

        for (int i = 0; i < count; i++) {
            int left = i * 2 + 0;
            int right = i * 2 + 1;

            buffer[left] = bc_evaluate(program_left);
            buffer[right] = bc_evaluate(program_right);

	        program_left->step_time(left_inv_sr);
	        program_right->step_time(right_inv_sr);
        }
    }

    static void bytecode_fill_planar(Bytecode_Program* program_left, Bytecode_Program* program_right, float* buffer, int sample_count) {
        float left_inv_sr = 1.0 / program_left->get_sample_rate();
        float right_inv_sr = 1.0 / program_right->get_sample_rate();

    	for (int i = 0; i < sample_count / 2; i++) {
    		buffer[i] = bc_evaluate(program_left);
    		program_left->step_time(left_inv_sr);
    	}
    	for (int i = sample_count / 2; i < sample_count; i++) {
    		buffer[i] = bc_evaluate(program_right);
    		program_right->step_time(right_inv_sr);
    	}
    }


    static float bc_evaluate(Bytecode_Program* program) {
        float result = bytecode_run(*program);
        result = flush_subnormal_sp(CLAMP(result, -1.0, 1.0));
        return result;
    }
}
