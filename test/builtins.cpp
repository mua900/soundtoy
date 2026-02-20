#include <cstring>
#include "api.h"

int main() {
	St_Sampler* sampler = st_sampler_create(60);

	const char* sin = "sin(t)";
	if (!st_sampler_set_expression(sampler, sin, strlen(sin))) {
		return 1;
	}

	const char* cos = "cos(t)";
	if (!st_sampler_set_expression(sampler, cos, strlen(cos))) {
		return 1;
	}

	const char* abs = "abs(t)";
	if (!st_sampler_set_expression(sampler, abs, strlen(abs))) {
		return 1;
	}

	const char* complex = "abs(sin(2.0*pi*t) + cos(t*t))";
	if (!st_sampler_set_expression(sampler, complex, strlen(complex))) {
		return 1;
	}

	return 0;
}
