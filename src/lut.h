#pragma once

#include "gate.h"

namespace atpg_ls {

class LUTCircuit;

class LUT {
public:
  LUT(Gate *gate, LUTCircuit *circuit);

  LUTCircuit *C;

  Gate *gate;

  bool isPI, isPO;

  uint32_t input_var;

  std::vector<LUT *> fanins;

  std::vector<LUT *> fanouts;
  std::vector<std::pair<LUT*, int>> fanouts_with_id;

  std::vector<LUT *> reigon;
  std::vector<LUT *> update_reigon;

  std::vector<Gate *> inner_gates;

  std::vector<Gate *> __gate_fanins;

  std::unordered_map<Gate *, int> input_id;

  int &value;
  void flip_value();

  const char *name;

  int *value_table;

  struct FaultInfo {
    int fd[2];
    int fpl[2];
    int value;
  } **fault_table;

  std::vector<FaultInfo *> fault_info;

  void init_lookup_table();

  // local search

  bool vsat;
  int vunat_cost;
  bool uptag;
  int up_cost;
  int fd[2];
  int fpl[2];
  // int is_good_var;
  int CC;

  int cal_value();
  void cal_fault_info(int *t_fd, int *t_fpl);
  void get_fault_info(Gate *gate, int *t_fd, int *t_fpl);

  // score
  double score;
  double score_value_unsatisfied_cost;
  double score_fault_detected_weight;
  double score_fault_propagated_weight;
  double score_fault_update_cost;

  void cal_score();
  void cal_update();
};

}
