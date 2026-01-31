#pragma once

#include "template.h"
#include "common.h"

struct Spectogram {
	Array<Array<float>> spectogram;

	void calculate_spectogram();
};
