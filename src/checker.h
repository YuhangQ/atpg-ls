#pragma once

#include "fault.h"
#include "pattern.h"
#include "circuit.h"

#include <set>

namespace atpg_ls {

int check_fault(const Pattern &p, const TMP_FAULT &f);
int final_check(const std::set<TMP_FAULT> &faults, const std::vector<Pattern> &patterns);
void init_final_checker();
    
};

