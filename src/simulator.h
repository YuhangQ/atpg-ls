#pragma once

#include "circuit.h"

namespace atpg_ls {

class Simulator : public Circuit {
public:
      void simulate(std::vector<LUT*> &inputs, int &score, int** fault_detected);
      int verify(LUTCircuit *lut_circuit, int** fault_detected);
};

}