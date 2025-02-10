#pragma once

#include "fault.h"

#include <vector>

namespace atpg_ls {

class Pattern {
public:

    std::vector<int> input_vector;
    std::vector<TMP_FAULT> detected_faults;

};

}