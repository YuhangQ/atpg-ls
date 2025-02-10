#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <assert.h>

namespace atpg_ls {

class LUT;

class Gate {
public:

    // gate basic info

    int id;
    int level;
    int rtopo;
    std::string name;
    enum Type { AND, NAND, OR, NOR, XOR, XNOR, NOT, BUF, INPUT, LINE } type;
    int value;
    bool isPI;
    bool isPO;
    std::vector<Gate*> fanouts;
    std::vector<Gate*> fanins;
    std::vector<Gate*> reigon;
    
    int avg_dist;

    int fault_propagated_len[2];
    int fault_propagated_weight[2];
    
    int fault_detected[2];
    int fault_detected_weight[2];

    bool is_propagated();

    int cal_value();

    bool cal_fault_detected(bool x);

    bool is_detected(Gate* one_of_input);

    int cal_propagate_len(bool x);

    LUT* parent_lut;
    int id_in_lut;

    int is_stem;

    void cal_fault_info(int fd[2], int fpl[2]);
    
    // ( partical ) score

    // score(x) = for y in neibor(x) { ~V_x,V_y make - ~V_x,V_y break }
    // [ cal_FPL(~Vx, fanouts) - cal_FPL(Vx, fanouts) ] * FLP_WEIGHT(x)
    // [ cal_FD(~Vx, fanouts) - cal_FD(Vx, fanouts)  ] * FD_WEIGHT(x)

    // [ cal_FPLS(~Vx, fanouts) - cal_FPLS(Vx, fanouts) ] * - FPLS_COST(x)
    // [ cal_FDS(~Vx, fanouts) - cal_FDS(Vx, fanouts) ] * - FDS_COST(x)
};

}