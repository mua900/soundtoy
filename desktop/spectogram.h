#pragma once

#include "template.h"
#include "common.h"

struct Signal {
	Array<float> samples;
};

struct Spectogram {
	Array<float> spectogram;
	int dim_x;
	int dim_y;
};

Spectogram calculate_spectogram(Signal signal);
