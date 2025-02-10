#include <set>

#include "lut.h"
#include "paras.h"

using namespace atpg_ls;

void LUT::flip_value() {
    value ^= 1;
    for(auto&[out, id] : fanouts_with_id) {
        out->input_var ^= (1 << id);
    }
}

void LUT::cal_fault_info(int *fd, int* fpl) {

    fd[0] = fd[1] = fpl[0] = fpl[1] = 0;
    if(isPO) {
        fd[!value] = 1;
        fd[value] = 0;
        return;
    }

    for(auto&[out, id] : fanouts_with_id) {
        if(!out->vsat) continue;

        FaultInfo &info = out->fault_table[id][out->input_var << 1 | out->value];

        int t_fd[2], t_fpl[2];

        t_fd[0] = info.fd[0] && out->fd[!out->value];
        t_fd[1] = info.fd[1] && out->fd[!out->value];

        t_fpl[0] = info.fpl[0] + info.fd[0] * out->fpl[!out->value];
        t_fpl[1] = info.fpl[1] + info.fd[1] * out->fpl[!out->value];

        fd[0] |= t_fd[0];
        fd[1] |= t_fd[1];

        fpl[0] = std::max(fpl[0], t_fpl[0]);
        fpl[1] = std::max(fpl[1], t_fpl[1]);
    }

    // std::vector<Gate*> reigon;

    // std::priority_queue<std::pair<Gate*, int>, std::vector<std::pair<Gate*, int>>, 
    //     std::function<bool(const std::pair<Gate*, int>&, const std::pair<Gate*, int>&)>> pq([](const std::pair<Gate*, int>& p1, const std::pair<Gate*, int>& p2) {
    //     return p1.first->id < p2.first->id;
    // });

    // for(Gate* out : gate->fanouts) {
    //     if(!out->vsat) continue;
    //     pq.push(std::make_pair(out, 0));
    // }

    // pq.push(std::make_pair(gate, 0));

    // while(!q.empty()) {
    //     auto [now, level] = q.front(); q.pop();

    //     for(Gate* out : now->fanouts) {
    //         if(!out->is_detected(now)) continue;

    //         if(level + 1 < OPT(lut)) {
    //             q.push(std::make_pair(out, level + 1));
    //         }
    //     }


    // }

}

int LUT::cal_value() {
    if(isPI) return value;
    return value_table[input_var];
}

LUT::LUT(Gate *gate, LUTCircuit *circuit):gate(gate), value(gate->value), name(gate->name.c_str()) {

    C = circuit;

    isPI = gate->isPI;
    isPO = gate->isPO;

    inner_gates.push_back(gate);

    std::set<Gate*> fanins;

    for(Gate* g : gate->fanins) {
        fanins.insert(g);
    }

    while(true) {

        std::vector<Gate*> candidates;
        for(Gate* fanin : fanins) {
            if(fanin->fanouts.size() >= 2) continue;
            if(fanin->isPI) continue;

            int add = 0;
            for(Gate* in : fanin->fanins) {
                if(fanins.count(in) == 0) {
                    add++;
                }
            }

            if(fanins.size() - 1 + add <= OPT(lut)) {
                candidates.push_back(fanin);
            }
        }

        if(candidates.size() == 0) break;

        Gate* random_gate = candidates[rand()%candidates.size()];

        inner_gates.push_back(random_gate);
        fanins.erase(random_gate);

        for(Gate* in : random_gate->fanins) {
            fanins.insert(in);
        }
    }

    for(Gate* in : fanins) {
        __gate_fanins.push_back(in);
    }
}


void LUT::init_lookup_table() {
    value_table = new int[1 << fanins.size()];
    fault_table = new FaultInfo *[fanins.size() + inner_gates.size()];
    for (int i = 0; i < (fanins.size() + inner_gates.size()); i++) {
        fault_table[i] = new FaultInfo[1 << (fanins.size() + 1)];
    }

    std::unordered_map<Gate*, int> gate2index;

    for(int i=0; i<fanins.size(); i++) {
        gate2index[fanins[i]->gate] = i;
    }
    for(int i=0; i<inner_gates.size(); i++) {
        gate2index[inner_gates[i]] =  i + fanins.size();
    }

    for (int i = 0; i < (1 << fanins.size()); i++) {
        for (int j = 0; j < fanins.size(); j++) {
            fanins[j]->gate->value = (i >> j) & 1;
        }
        for (Gate *g : inner_gates) {
            g->value = g->cal_value();
        }

        assert(inner_gates[inner_gates.size() - 1] == gate);

        value_table[i] = gate->value;

        for(gate->value=0; gate->value<=1; gate->value++) {

            for(auto&[g, index] : gate2index) {
                g->fault_detected[0] = g->fault_detected[1] = 0;
                g->fault_propagated_len[0] = g->fault_propagated_len[1] = 0;
            }

            gate->fault_detected[!gate->value] = 1;
            gate->fault_detected[gate->value] = 0;
            gate->fault_propagated_len[0] = 0;
            gate->fault_propagated_len[1] = 0;

            std::queue<Gate*> q;
            q.push(gate);

            while(!q.empty()) {
                Gate *now = q.front(); q.pop();

                FaultInfo *info = &fault_table[gate2index[now]][(i<<1)|gate->value];
                info->fd[0] = now->fault_detected[0];
                info->fd[1] = now->fault_detected[1];
                info->fpl[0] = now->fault_propagated_len[0];
                info->fpl[1] = now->fault_propagated_len[1];
                info->value = now->value;

                // if(now->name == "new_n22_") {
                //     printf("Gate: %s value: %d inputs: [ ", now->name.c_str(), now->value);
                //     for(Gate *in : now->fanins) {
                //         printf("%s ", in->name.c_str());
                //     }
                //     printf("]\n");
                // }

                for(Gate *in : now->fanins) {
                    if(gate2index.count(in) == 0) continue;

                    if(now->is_detected(in)) {
                        in->fault_detected[!in->value] = std::max(in->fault_detected[!in->value], now->fault_detected[!now->value]);
                        in->fault_propagated_len[!in->value] = std::max(in->fault_propagated_len[!in->value], now->fault_propagated_len[!now->value] + 1);
                    }

                    q.push(in);
                }
            }
        }
    }
}
