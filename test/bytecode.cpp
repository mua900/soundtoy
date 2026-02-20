#include <cstring>
#include "api.h"

int main() {
	St_Sampler* sampler = st_sampler_create(60);

	const char* input = "sin(t)";
	if (!st_sampler_set_expression(sampler, input, strlen(input))) {
		return 1;
	}

	st_sampler_set_sample_time(sampler, 2*3.1415);
	float result = st_sampler_evaluate(sampler);
	if (result > 0.01) {
		return 1;
	}

	return 0;
}
