#include <set>
#include <assert.h>

#include "circuit.h"
#include "paras.h"

using namespace atpg_ls;

void LUTCircuit::print() {

    std::set<Gate*> st;

    int total_gate = 0;

    printf("PIs: ( ");
    for(LUT* lut : PIs) {
        printf("%s ", lut->gate->name.c_str());
    }
    printf(")\n");
    printf("POs: ( ");
    for(LUT* lut : POs) {
        printf("%s ", lut->gate->name.c_str());
    }
    printf(")\n");

    

    for(LUT* lut : luts) {

        printf("[v:%d vs:%d fd0:%d fd1:%d] ", lut->value, lut->vsat, lut->fd[0], lut->fd[1]);
        printf("lut: %s inputs: ( ", lut->gate->name.c_str());

        total_gate += lut->inner_gates.size();

        for(LUT* in : lut->fanins) {
            printf("%s ", in->gate->name.c_str());
        }
        printf(")\n");
    }

    printf("total gate: %d\n", total_gate);
}





LUTCircuit* Circuit::build_lut_circuit() {

    LUTCircuit* C = new LUTCircuit();
    C->C = this;

    std::unordered_map<Gate*, LUT*> gate2LUT;

    std::queue<Gate*> q;
    std::set<Gate*> done;

    for(Gate* gate : gates) {
        // assert fanouts >= 2
        if(gate->fanouts.size() >= 2 || gate->isPI || (gate->isPO && gate->fanouts.size() == 0)) {
            q.push(gate);
            done.insert(gate);
        }
    }

    while(!q.empty()) {
        Gate* gate = q.front();
        q.pop();

        LUT* lut = new LUT(gate, C);
        
        gate2LUT[gate] = lut;
        
        // printf("size: %d lut: %s inputs: ( ", lut->inner_gates.size(), lut->gate->name.c_str());
        // for(Gate* in : lut->__gate_fanins) {
        //     printf("%s ", in->name.c_str());
        // }
        // printf(") [");

        // for(Gate* inner : lut->inner_gates) {
        //     printf("%s ", inner->name.c_str());
        // }
        // printf("]\n");

        for(Gate* in : lut->__gate_fanins) {
            if(in->fanouts.size() < 2 && !in->isPI && done.count(in) == 0) {
                done.insert(in);
                q.push(in);
            }
        }

        C->luts.push_back(lut);
    }

    for(LUT* lut : C->luts) {

        lut->name = lut->gate->name.c_str();


        std::sort(lut->inner_gates.begin(), lut->inner_gates.end(), [](Gate* a, Gate* b) {
            return a->id < b->id;
        });

        for(Gate* in : lut->__gate_fanins) {
            assert(gate2LUT.count(in) > 0);
            
            lut->fanins.push_back(gate2LUT[in]);
            gate2LUT[in]->fanouts.push_back(lut);
        }

        if(lut->isPI) {
            C->PIs.push_back(lut);
            lut->isPI = 1;
        }
        if(lut->isPO) {
            C->POs.push_back(lut);
            lut->isPO = 1;
        }
    }

    for(LUT* lut : C->luts) {
        C->rtopo_luts.push_back(lut);
    }

    std::sort(C->luts.begin(), C->luts.end(), [](LUT* a, LUT* b) {
        return a->gate->id < b->gate->id;
    });

    std::sort(C->rtopo_luts.begin(), C->rtopo_luts.end(), [](LUT* a, LUT* b) {
        return a->gate->rtopo < b->gate->rtopo;
    });

    for(LUT* lut : C->luts) {
        for(int i=0; i<lut->inner_gates.size(); i++) {
            Gate *g = lut->inner_gates[i];
            g->parent_lut = lut;
            g->id_in_lut = lut->fanins.size() + i;
        }
    }

    for(LUT* lut : C->luts) {
        for(int i=0; i<lut->fanins.size(); i++) {
            LUT* in = lut->fanins[i];
            in->fanouts_with_id.push_back(std::make_pair(lut, i));
        }
    }

    for(LUT* lut : C->luts) {
        assert(lut->fanouts.size() == lut->fanouts_with_id.size());
    }

    return C;
}



void Circuit::insert_lines_for_stem() {

    for(int j=0; j<gates.size(); j++) {

        Gate* g = gates[j];

        if(g->type == Gate::LINE) break;

        g->is_stem = g->isPO ? g->fanouts.size() >= 1 : g->fanouts.size() >= 2;
        if(!g->is_stem) continue;

        if(g->isPO) {
            Gate* line = new Gate();
            line->name = g->name + "_line_PO";
            line->type = Gate::LINE;
            line->isPI = false;
            line->isPO = true;
            line->is_stem = false;

            line->fanins.push_back(g);
            g->fanouts.push_back(line);

            g->isPO = false;
            POs.erase(std::find(POs.begin(), POs.end(), g));
            POs.push_back(line);
            
            gates.push_back(line);
            name2gate.insert(std::make_pair(line->name, line));
        }

        printf(">>>> now: %s\n", g->name.c_str());

        printf("outs: [ ");
        for(Gate* out :g->fanouts){
            printf("%s ", out->name.c_str());
        }
        printf("]\n");

        for(int i=0; i<g->fanouts.size(); i++) {
            Gate* out = g->fanouts[i];

            if(out->type == Gate::LINE) break;

            printf("        g_name: %s  outname: %s\n", g->name.c_str(), out->name.c_str());

            Gate* line = new Gate();
            line->name = g->name + "_line_" + out->name;
            line->type = Gate::LINE;
            line->isPI = false;
            line->isPO = false;

            line->fanins.push_back(g);
            line->fanouts.push_back(out);

            out->fanins.erase(std::find(out->fanins.begin(), out->fanins.end(), g));
            out->fanins.push_back(line);

            g->fanouts.erase(std::find(g->fanouts.begin(), g->fanouts.end(), out));
            g->fanouts.push_back(line);

            gates.push_back(line);
            name2gate.insert(std::make_pair(line->name, line));

            i--;
        }

    }

}

void Circuit::init_avg_dist() {

    int *now_dist = new int[gates.size() + 1] { 0 };
    int *total_dist = new int[gates.size() + 1] { 0 };
    int *total_cnt = new int[gates.size() + 1] { 0 };
    // int *topo_cnt = new int[gates.size() + 1] { 0 };

    // memset(total_dist, 0, sizeof(int) * (gates.size() + 1));

    for(Gate* po : POs) {
        // memset(topo_cnt, 0, sizeof(int) * (gates.size() + 1));
        // memset(now_dist, 0x3f, sizeof(int) * (gates.size() + 1));
        for(Gate* g : gates) {
            if(g->isPO) {
                now_dist[g->id] = 0;
            } else {
                now_dist[g->id] = 0x3f3f3f3f;
            }
        }

        // printf(">> po: %s\n", po->name.c_str());
    
        std::queue<Gate*> q;
        q.push(po);

        while(!q.empty()) {
            Gate* u = q.front(); q.pop();

            total_dist[u->id] += now_dist[u->id];
            total_cnt[u->id] ++;

            // printf("now: %s\n", u->name.c_str());

            for(Gate* in : u->fanins) {
                if(now_dist[u->id] + 1 < now_dist[in->id]) {
                    now_dist[in->id] = now_dist[u->id] + 1;
                    q.push(in);
                }
            }

            // printf("dist: %d\n", total_dist[name2gate["G132"]->id]);
        }
    }

    for(Gate* g : gates) {

        if(total_cnt[g->id] <= 0) {
            printf("ERROR: gate: %s total: %d cnt: %d\n", g->name.c_str(), total_dist[g->id], total_cnt[g->id]);
            exit(-1);
        }

        assert(total_cnt[g->id] > 0);
        
        g->avg_dist = total_dist[g->id] / total_cnt[g->id];

        printf("ERROR: gate: %s total: %d cnt: %d\n", g->name.c_str(), total_dist[g->id], total_cnt[g->id]);

        // if(g->id)
        if(!g->isPO) assert(g->avg_dist > 0);
    }

    delete [] now_dist;
    delete [] total_dist;
    delete [] total_cnt;
}

void Circuit::init_topo_index() {
    int topo = 1;
    std::queue<Gate*> q;

    std::unordered_map<Gate*, int> ins;
    for(Gate* gate : gates) {
        ins[gate] = gate->fanins.size();
    }

    for(auto in : PIs) {
        in->id = topo++;
        q.push(in);
    }

    while(!q.empty()) {
        Gate* g = q.front(); q.pop();
        for(Gate* out : g->fanouts) {
            ins[out]--;
            if(ins[out] == 0) {
                out->id = topo++;
                q.push(out);
            }
        }
    }

    // 计算反向拓扑序
    topo = 1;
    std::unordered_map<Gate*, int> outs;

    for(Gate* g : gates) {
        if(g->fanouts.size() == 0) {
            g->rtopo = topo++;
            q.push(g);
        }
    }

    for(Gate* gate : gates) {
        outs[gate] = gate->fanouts.size();
    }

    std::unordered_set<Gate*> ok;

    while(!q.empty()) {
        Gate* g = q.front(); q.pop();
        rtopo_gates.push_back(g);
        ok.insert(g);
        for(Gate* in : g->fanins) {
            outs[in]--;

            if(outs[in] == 0) {
                in->rtopo = topo++;
                q.push(in);
            }
        }
    }

    std::sort(gates.begin(), gates.end(), [](Gate* a, Gate* b) {
        return a->id < b->id;
    });
}


void Circuit::print() {

    printf("PIs: ( ");
    for(Gate* gate : PIs) {
        printf("%s ", gate->name.c_str());
    }
    printf(")\n");

    const char *name[10] = {"AND", "NAND", "OR", "NOR", "XOR", "XNOR", "NOT", "BUF", "INPUT", "LINE"};

    for(Gate* g : gates) {
        printf("[sa0: %d sa1: %d v: %d vsat: %d] %s = %s (",g->fault_detected[0], g->fault_detected[1], g->value, g->cal_value() == g->value,  g->name.c_str(), name[g->type]);
        for(Gate* in : g->fanins) {
            printf("%s ", in->name.c_str());
        }
        printf(")\n");
    }

    printf("POs: ( ");
    for(Gate* gate : POs) {
        printf("%s ", gate->name.c_str());
    }
    printf(")\n");
}

double LUTCircuit::check() {

    // static bool init = 0;
    // static std::unordered_set<Gate*> dt;

    printf("checking circuit ...\n");

    double score_value_unsatisfied_cost = 0;
    double score_fault_detected_weight = 0;
    double score_fault_propagated_weight = 0;
    double score_fault_update_cost = 0;
    int unsatisfied_lut = 0;

    for(LUT* lut : luts) {
        assert(lut->vsat == (lut->value == lut->cal_value()));

        if(!lut->vsat) {
            score_value_unsatisfied_cost += lut->vunat_cost;
            unsatisfied_lut++;
            // printf("vunsat: %s\n", lut->name);
        }

        if(lut->uptag) {
            score_fault_update_cost += lut->up_cost;
        } else {
            int t_fd[2], t_fpl[2];
            lut->cal_fault_info(t_fd, t_fpl);
            assert(t_fd[0] == lut->fd[0]);
            assert(t_fd[1] == lut->fd[1]);
            assert(t_fpl[0] == lut->fpl[0]);
            assert(t_fpl[1] == lut->fpl[1]);
        }

        int input = 0;
        for(int i=0; i<lut->fanins.size(); i++) {
            input |= (lut->fanins[i]->value << i);
        }
        input <<= 1;
        input |= lut->value;

        for(int i=lut->fanins.size(); i<lut->fanins.size()+lut->inner_gates.size(); i++) {
            LUT::FaultInfo &info = lut->fault_table[i][input];
            Gate* g = lut->inner_gates[i-lut->fanins.size()];

            int t_fd[2], t_fpl[2];
            t_fd[0] = info.fd[0] && lut->fd[!lut->value];
            t_fd[1] = info.fd[1] && lut->fd[!lut->value];
            t_fpl[0] = info.fpl[0] + info.fd[0] * lut->fpl[!lut->value];
            t_fpl[1] = info.fpl[1] + info.fd[1] * lut->fpl[!lut->value];
            
            score_fault_detected_weight += t_fd[0] * g->fault_detected_weight[0];
            score_fault_detected_weight += t_fd[1] * g->fault_detected_weight[1];

            if(!g->isPO) {
                score_fault_propagated_weight += (double)t_fpl[0] / g->avg_dist * g->fault_detected_weight[0];
                score_fault_propagated_weight += (double)t_fpl[1] / g->avg_dist * g->fault_detected_weight[1];
            }
        }
    }

    printf("=====================================\n");
    printf("unsat_lut: %d\n", unsatisfied_lut);
    printf("score_value_unsatisfied_cost: %.2f\n", score_value_unsatisfied_cost);
    printf("score_fault_detected_weight: %.2f\n", score_fault_detected_weight);
    printf("score_fault_propagated_weight: %.2f\n", score_fault_propagated_weight);
    printf("score_fault_update_cost: %.2f\n", score_fault_update_cost);
    
    double score = - score_value_unsatisfied_cost + score_fault_detected_weight + score_fault_propagated_weight - score_fault_update_cost;

    printf("score: %d\n", score);

    return score;
}