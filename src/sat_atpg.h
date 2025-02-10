#pragma once

#include "circuit.h"

using atpg_ls::TMP_FAULT;

bool sat_atpg(const TMP_FAULT &fal, std::vector<int> &input_vector);
void sat_atpg_init(const char* file);