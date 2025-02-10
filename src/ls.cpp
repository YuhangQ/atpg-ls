#include <set>
#include <chrono>
#include <iostream>
#include <map>

#include "circuit.h"
#include "paras.h"
#include "simulator.h"
#include "sat_atpg.h"
#include "pattern.h"
#include "checker.h"

using namespace atpg_ls;

LUT* LUTCircuit::ls_pick() {

    LUT* pick = nullptr;
    
    // 采样
    for(int i=0; i<OPT(t); i++) {
        LUT* lut = luts[rand() % luts.size()];
        if(!lut->CC) continue;
        lut->cal_score();
        if(lut->score <= 0) continue;
        if(pick == nullptr || lut->score > pick->score) {
            pick = lut;
        }
    }

    if(pick != nullptr) {
        return pick;
    }

    std::vector<LUT*> unsat;

    // 动态加权
    ls_update(unsat);

    if(unsat.size() == 0) return nullptr;

    pick = unsat[rand() % unsat.size()];

    if(pick->fanins.size() == 0) {
        return pick;
    }

    if(rand() % 10000 < 3000) {
        return pick->fanins[rand() % pick->fanins.size()];
    }
    return pick;
}

void LUTCircuit::ls_flip(LUT *lut) {

    lut->CC = 0;
    lut->uptag = 0;
    lut->flip_value();
    if(!lut->isPI) lut->vsat = !lut->vsat;

    for(LUT* out : lut->fanouts) {
        out->CC = 1;
        out->vsat = (out->cal_value() == out->value);
    }

    lut->cal_fault_info(lut->fd, lut->fpl);

    for(LUT* r : lut->reigon) {
        r->CC = 1;
        int t_fd[2], t_fpl[2];
        r->cal_fault_info(t_fd, t_fpl);

        if(t_fd[0] != r->fd[0] || t_fd[1] != r->fd[1] || t_fpl[0] != r->fpl[0] || t_fpl[1] != r->fpl[1]) {
            r->uptag = 1;
        }
    }
}

void LUTCircuit::ls_update(std::vector<LUT*> &unsat) {

    if(rand() % 10000 <= OPT(sp) * 10000) {

        for(LUT* lut : luts) {
            if(lut->vsat) {
                if(lut->vunat_cost - OPT(vsat_inc) >= 1) {
                    lut->vunat_cost -= OPT(vsat_inc);
                }
            }
            if(!lut->vsat) {
                unsat.push_back(lut);
            }
            if(!lut->uptag) {
                if(lut->up_cost - OPT(up_inc) >= 1) {
                    lut->up_cost -= OPT(up_inc);
                }
            }
            for(int i=lut->fanins.size(); i<lut->fanins.size()+lut->inner_gates.size(); i++) {
                Gate* g = lut->inner_gates[i-lut->fanins.size()];
                LUT::FaultInfo &fi = lut->fault_table[i][lut->input_var];
                if(fi.fd[0] && g->fault_detected_weight[0] - OPT(fw_inc) >= 1) {
                    g->fault_detected_weight[0] -= OPT(fw_inc);
                }
                if(fi.fd[1] && g->fault_detected_weight[1] - OPT(fw_inc) >= 1) {
                    g->fault_detected_weight[1] -= OPT(fw_inc);
                }
                if(fi.fd[0] && g->fault_propagated_weight[0] - OPT(fw_inc) >= 1) {
                    g->fault_propagated_weight[0] -= OPT(fw_inc);
                }
                if(fi.fd[1] && g->fault_propagated_weight[1] + OPT(fw_inc) >= 1) {
                    g->fault_propagated_weight[1] -= OPT(fw_inc);
                }
            }
        }
        
    } else {

        for(LUT* lut : luts) {
            if(!lut->vsat) {

                // printf("ocost: %d add: %d\n", lut->vunat_cost);
                if(lut->vunat_cost + OPT(vsat_inc) > OPT(vsat_max)) {
                    lut->vunat_cost = OPT(vsat_max);
                } else {
                    lut->vunat_cost += OPT(vsat_inc);
                }
                // printf("cost: %d add: %d\n", lut->vunat_cost, OPT(vsat_inc));
                
                // for(LUT* out : lut->fanouts) {
                //     if(out->vunat_cost -= OPT(vsat_inc) > 1) {
                //         out->vunat_cost -= OPT(vsat_inc);
                //     } else {
                //         out->vunat_cost = 1;
                //     }
                // }
                
                unsat.push_back(lut);
            }
            if(lut->uptag) {
                if(lut->up_cost + OPT(up_inc) > OPT(up_max)) {
                    lut->up_cost = OPT(up_max);
                } else {
                    lut->up_cost += OPT(up_inc);
                }
            }
            for(int i=lut->fanins.size(); i<lut->fanins.size()+lut->inner_gates.size(); i++) {
                Gate* g = lut->inner_gates[i-lut->fanins.size()];
                LUT::FaultInfo &fi = lut->fault_table[i][lut->input_var];
                if(!fi.fd[0] && g->fault_detected_weight[0] > 0 && g->fault_detected_weight[0] + OPT(fw_inc) <= OPT(fw_max)) {
                    g->fault_detected_weight[0] += OPT(fw_inc);
                }
                if(!fi.fd[1] && g->fault_detected_weight[1] > 0 && g->fault_detected_weight[1] + OPT(fw_inc) <= OPT(fw_max)) {
                    g->fault_detected_weight[1] += OPT(fw_inc);
                }
                if(!fi.fd[0] && g->fault_propagated_weight[0] > 0 && g->fault_propagated_weight[0] + OPT(fw_inc) <= OPT(fw_max)) {
                    g->fault_propagated_weight[0] += OPT(fw_inc);
                }
                if(!fi.fd[1] && g->fault_propagated_weight[0] > 0 && g->fault_propagated_weight[1] + OPT(fw_inc) <= OPT(fw_max)) {
                    g->fault_propagated_weight[1] += OPT(fw_inc);
                }
            }
        }
    }
}

void LUTCircuit::ls_main() {

    printf("====== local search start ======\n");

    printf("initing lookup table ...\n");
    ls_init();

    printf("ganerating fault info ...\n");

    int num_gates = 0;
    int num_total_fault = 0;
    int num_detected_fault = 0;
    int num_undetected_fault = 0;
    int num_pattern = 0;

    for(LUT* lut : luts) {
        num_gates += lut->inner_gates.size();
    }
    assert(num_gates == C->gates.size());
    fault_detected = new int*[num_gates];
    for(int i=0; i<num_gates; i++) {
        fault_detected[i] = new int[2];
        fault_detected[i][0] = fault_detected[i][1] = 0;
    }
    num_total_fault = num_undetected_fault = num_gates * 2;

    printf("staring local search ...\n");
    
    std::queue<TMP_FAULT> faults;

    for(Gate* g : C->gates) {

        std::string name = g->name;

        if(g->is_stem) {
            faults.push(TMP_FAULT{g, name, 0, 1, 0});
            faults.push(TMP_FAULT{g, name, 1, 1, 0});
        } else {

            if(g->isPO) {
                faults.push(TMP_FAULT{g, name, 0, g->type != Gate::LINE, g->type == Gate::LINE});
                faults.push(TMP_FAULT{g, name, 1, g->type != Gate::LINE, g->type == Gate::LINE});
                continue;
            }

            assert(g->fanouts.size() == 1);
            Gate *fanout = g->fanouts[0];

            int stem = (!g->isPI) && (g->type != Gate::LINE);

            if(fanout->type == Gate::Type::BUF || fanout->type == Gate::Type::NOT) {
                continue;
            } else if(fanout->type == Gate::Type::XOR || fanout->type == Gate::Type::XNOR) {
                faults.push(TMP_FAULT{g, name, 0, stem, 0});
                faults.push(TMP_FAULT{g, name, 1, stem, 0});
            } else if(fanout->type == Gate::Type::NAND || fanout->type == Gate::Type::AND) {
                faults.push(TMP_FAULT{g, name, 1, stem, 0});
            } else if(fanout->type == Gate::Type::NOR || fanout->type == Gate::Type::OR) {
                faults.push(TMP_FAULT{g, name, 0, stem, 0});
            } else {
                assert(false);
            }
        }
    }

    printf("fault-size: %d\n", faults.size());

    std::vector<TMP_FAULT> t_faults;
    std::set<TMP_FAULT> t_verify_set;

    while(!faults.empty()) {
        TMP_FAULT f = faults.front();
        faults.pop();

        assert(t_verify_set.count(f) == 0);

        t_faults.push_back(f);
        t_verify_set.insert(f);
    }

    std::map<std::pair<std::string, int>, std::pair<int, int>> mp;

    for(TMP_FAULT &f : t_faults) {
        faults.push(f);
        mp[std::make_pair(f.g->name, f.stuck_at)] = std::make_pair(f.is_stem, f.is_PO);
    }

    printf("fault-size: %d\n", t_faults.size());
    printf("verify-size: %d\n", t_verify_set.size());
    assert(t_faults.size() == t_verify_set.size());

    std::vector<Pattern> patterns;

    auto start = std::chrono::high_resolution_clock::now();

    while(!faults.empty()) {

        while(!faults.empty()) {
            TMP_FAULT f = faults.front();
            if(fault_detected[f.g->id-1][f.stuck_at]) {
                faults.pop();
            } else {
                break;
            }
        }

        if(faults.empty()) break;

        ls_random_sol();

        TMP_FAULT f = faults.front(); faults.pop();
        Gate* g = f.g;
        int stuck_at = f.stuck_at;

        printf("start with fault: %s SA%d\t", g->name.c_str(), stuck_at);

        std::vector<int> inputs;

        printf("verify ...");

        int res1 = sat_atpg(f, inputs);
        
        if(res1 == 0) {
            printf(" unsat!\n");
            continue;
        }

        printf("sat !\n");

        assert(inputs.size() == PIs.size());

        for(int i=0; i<inputs.size(); i++) {
            PIs[i]->value = inputs[i];
        }

        printf("sim: %s %d\n", g->name.c_str(), stuck_at);
        for(int i=0; i<inputs.size(); i++) {
            printf("%d ", inputs[i]);
        }
        printf("\n");

        for(LUT* lut : luts) {
            lut->input_var = 0;
            for(int i=0; i<lut->fanins.size(); i++) {
                LUT* in = lut->fanins[i];
                lut->input_var |= (in->value << i);
            }
        }

        ls_gen_sol(f);
        
        int score;
        simulator->simulate(PIs, score, fault_detected);

        Pattern pattern;
        for(LUT* lut : PIs) {
            pattern.input_vector.push_back(lut->value);
        }

        assert(check_fault(pattern, f));

        simulator->name2gate[g->name]->fault_detected[stuck_at] = 1;
        assert(fault_detected[g->id-1][stuck_at] == 0);

        int num_fault = 0;

        // check if real detected
        for(Gate* g : simulator->gates) {
            for(int i=0; i<=1; i++) {
                if(g->fault_detected[i] && !fault_detected[g->id-1][i]) {
                    fault_detected[g->id-1][i] = 1;

                    if(mp.count(std::make_pair(g->name, i))) {
                        auto [is_tem, is_PO] = mp[std::make_pair(g->name, i)];

                        TMP_FAULT f = TMP_FAULT{g, g->name, i, is_tem, is_PO};

                        int res = check_fault(pattern, f);
                        if(!res) {
                            printf("fault: %s SA%d is not real detected!\n", g->name.c_str(), i);
                            fault_detected[g->id-1][i] = 0;
                        } else {
                            pattern.detected_faults.push_back(f);
                        }
                    }

                    if(fault_detected[g->id-1][i]) {
                        num_detected_fault++;
                        num_undetected_fault--;
                        num_fault++;
                    }
                }
            }
        }

        patterns.push_back(pattern);
        assert(num_fault > 0);

        if(num_fault > 0) {
            num_pattern++;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // cnt++;
        printf("Cover: %.2f%% pattern: %d new_detected: %d undected: %d time: %.2fs\n", (double)num_detected_fault / num_total_fault * 100, num_pattern, num_fault, num_undetected_fault, (double)duration/1000);

        // break;
    }

    for(int i=0; i<num_gates; i++) {
        if(fault_detected[i][0] == 0) {
            printf("undetected: %s SA0\n", simulator->gates[i]->name.c_str());
        }
        if(fault_detected[i][1] == 0) {
            printf("undetected: %s SA1\n", simulator->gates[i]->name.c_str());
        }
    }

    printf("====== local search end ======\n");

    final_check(t_verify_set, patterns);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Execution time: " << duration << " milliseconds" << std::endl;
}

void LUTCircuit::ls_gen_sol(const TMP_FAULT &target) {

    std::vector<int> best_sol;
    int best_score = -1;
    simulator->simulate(PIs, best_score, fault_detected);
    for(LUT* lut : PIs) {
        best_sol.push_back(lut->value);
    }
    
    for(int step=0; step<OPT(max_step_coeff)*luts.size(); step++) {
        // printf("step: %d\n", step);

        LUT* pick = ls_pick();

        if(pick == nullptr) {
            pick = luts[rand() % luts.size()];
        }

        // pick->cal_score();

        // double t1 = check();

        // printf(">>>>>>>>>>>>>\n");
        // printf("dert_score: %.2f\n", pick->score);
        // printf("dert_unsat_cost: %.2f\n", pick->score_value_unsatisfied_cost);
        // printf("dert_fault_detected_weight: %.2f\n", pick->score_fault_detected_weight);
        // printf("dert_fault_propagated_weight: %.2f\n", pick->score_fault_propagated_weight);
        // printf("dert_up_cost: %.2f\n", pick->score_fault_update_cost);

        ls_flip(pick);

        // double t2 = check();
        // assert(((t2 - t1) - pick->score) < 1e-6);

    
        if(pick->isPI) {
            
            // printf("step: %d score: %d\n", step, score);

            Pattern p;
            for(LUT* lut : PIs) {
                p.input_vector.push_back(lut->value);
            }
            int res = check_fault(p, target);

            if(!res) {
                ls_flip(pick);
                pick->CC = 0;
            } else {
                int score;
                simulator->simulate(PIs, score, fault_detected);
                if(score > best_score) {
                    best_score = score;
                    best_sol.clear();
                    for(LUT* lut : PIs) {
                        best_sol.push_back(lut->value);
                    }
                }
            }
        }
    }

    assert(best_score != -1);

    for(int i=0; i<PIs.size(); i++) {
        PIs[i]->value = best_sol[i];
    }

    for(LUT* lut : luts) {
        lut->input_var = 0;
        for(int i=0; i<lut->fanins.size(); i++) {
            LUT* in = lut->fanins[i];
            lut->input_var |= (in->value << i);
        }
        lut->value = lut->cal_value();
        lut->vsat = 1;
    }

    for(LUT* lut : rtopo_luts) {
        lut->cal_fault_info(lut->fd, lut->fpl);
    }
}


void LUTCircuit::ls_init() {
    for(LUT* lut : luts) {
        static int cnt = 0;
        printf("[%d/%d]\n", ++cnt, luts.size());
        lut->init_lookup_table();
    }

    for(LUT* lut : luts) {
        for(LUT* out :lut->fanouts) {
            for(int i=0; i<out->fanins.size(); i++) {
                if(out->fanins[i] == lut) {
                    lut->fault_info.push_back(out->fault_table[i]);
                }
            }
        }
    }

    for(LUT* lut : luts) {
        std::set<LUT*> t_reigon;
        for(LUT* out : lut->fanouts) {
            for(LUT* in : out->fanins) {
                t_reigon.insert(in);
            }
        }
        for(LUT* in : lut->fanins) {
            t_reigon.insert(in);
        }
        t_reigon.erase(lut);
        for(LUT* r : t_reigon) {
            lut->reigon.push_back(r);
        }
    }

    for(LUT* lut : luts) {
        std::set<LUT*> t_reigon;
        for(LUT* r: lut->reigon) {
            t_reigon.insert(r);
            for(LUT* out2 : r->fanouts) {
                t_reigon.insert(out2);
                for(LUT* in2 : out2->fanins) {
                    t_reigon.insert(in2);
                }
            }
        }
        for(LUT* r : lut->fanouts) {
            t_reigon.insert(r);
        }
        for(LUT* r : t_reigon) {
            lut->update_reigon.push_back(r);
        }
    }
}

void LUTCircuit::ls_random_sol() {

    step = 0;

    std::vector<int*> t_focus;

    for(LUT* lut : luts) {
        lut->up_cost = 1;

        lut->vunat_cost = 1;
        if(lut->isPI) {
            lut->vunat_cost = OPT(vsat_inc) * 100;
        }

        lut->CC = 1;
        for(Gate* g : lut->inner_gates) {

            g->fault_detected_weight[0] = !fault_detected[g->id-1][0];
            g->fault_detected_weight[1] = !fault_detected[g->id-1][1];

            if(g->fault_detected_weight[0]) {
                t_focus.push_back(&g->fault_detected_weight[0]);
            }

            if(g->fault_detected_weight[1]) {
                t_focus.push_back(&g->fault_detected_weight[1]);
            }
            
            g->fault_propagated_weight[0] = !fault_detected[g->id-1][0];
            g->fault_propagated_weight[1] = !fault_detected[g->id-1][1];
        }
    }

    int *tw = t_focus[rand()%t_focus.size()];
    *tw = 100000;
    
    for(LUT* lut : luts) {
        lut->uptag = 0;
        lut->value = rand() % 2;
    }

    for(LUT* lut : luts) {
        lut->input_var = 0;
        for(int i=0; i<lut->fanins.size(); i++) {
            LUT* in = lut->fanins[i];
            lut->input_var |= (in->value << i);
        }
    }

    for(LUT* lut : luts) {
        lut->vsat = (lut->value == lut->cal_value());
    }

    for(LUT* lut : rtopo_luts) {
        lut->cal_fault_info(lut->fd, lut->fpl);
    }

    for(LUT* lut : luts) {
        lut->cal_score();
    }
}