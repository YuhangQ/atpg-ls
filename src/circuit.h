#pragma once

#include "fault.h"
#include "lut.h"

using ll = long long;

namespace atpg_ls {

class Simulator;
class Circuit;

class LUTCircuit {
public:
std::vector<LUT*> PIs;
std::vector<LUT*> POs;
std::vector<LUT*> luts;
std::vector<LUT*> rtopo_luts;

int** fault_detected;

void print();

// local search
void ls_update(std::vector<LUT*> &unsat);
LUT* ls_pick();
void ls_flip(LUT *lut);
void ls_main();
void ls_init();
void ls_random_sol();
void ls_gen_sol(const TMP_FAULT &target);

// checker
double check();

Simulator *simulator;
Circuit *C;

int step;

};

class Circuit {
public:
std::vector<Gate*> PIs;
std::vector<Gate*> POs;
std::vector<Gate*> gates;
std::vector<Gate*> rtopo_gates;

std::unordered_map<std::string, Gate*> name2gate;

void parse_from_file(const char *filename);
void insert_lines_for_stem();

void print();

LUTCircuit* build_lut_circuit();

void init_topo_index();
void init_avg_dist();

};

}