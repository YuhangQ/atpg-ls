#include "circuit.h"

#include "assert.h"

using namespace atpg_ls;

int Gate::cal_propagate_len(bool x) {

    int fpl[2];
    fpl[0] = fpl[1] = 0;
    if(isPO) {
        fpl[!value] = 0;
        fpl[value] = 0;
        return fpl[x];
    }

    for(Gate* out : fanouts) {
        if(!out->is_detected(this)) continue;
        fpl[!value] = std::max(fpl[!value], out->fault_propagated_len[!out->value] + 1);
    }
    
    return fpl[x];
}

bool Gate::is_detected(Gate* one_of_input) {
    one_of_input->value = !one_of_input->value;
    bool detect = (cal_value() != value);
    one_of_input->value = !one_of_input->value;
    return (cal_value() == value) && detect;
}

bool Gate::is_propagated() {
    return fault_detected[0] || fault_detected[1];
}

bool Gate::cal_fault_detected(bool x) {
    if(isPO) {
        if(x == !value) return 1;
        else return 0;
    }

    bool sa0 = 0;
    bool sa1 = 0;

    for(Gate* out : fanouts) {
        if(!out->is_propagated()) continue;
        if(out->cal_value() != out->value) continue;

        this->value = !this->value;
        bool detect = (out->cal_value() != out->value);
        this->value = !this->value;

        if(!detect) continue;

        sa0 |= this->value;
        sa1 |= !this->value;
    }
    if(x == 0) return sa0;
    else return sa1;
}

int Gate::cal_value() {
    int res;

    switch(type) {
        case NOT:
            res = !fanins[0]->value;
            break;
        case LINE:
            // pass through
        case BUF:
            res = fanins[0]->value;
            break;
        case AND:
            res = fanins[0]->value;
            for(int i=1; i<fanins.size(); i++) {
                res &= fanins[i]->value;
            }
            break;
        case NAND:
            res = fanins[0]->value;
            for(int i=1; i<fanins.size(); i++) {
                res &= fanins[i]->value;
            }
            res = !res;
            break;
        case OR:
            res = fanins[0]->value;
            for(int i=1; i<fanins.size(); i++) {
                res |= fanins[i]->value;
            }
            break;
        case NOR:
            res = fanins[0]->value;
            for(int i=1; i<fanins.size(); i++) {
                res |= fanins[i]->value;
            }
            res = !res;
            break;
        case XOR:
            res = fanins[0]->value;
            for(int i=1; i<fanins.size(); i++) {
                res ^= fanins[i]->value;
            }
            break;
        case XNOR:
            res = fanins[0]->value;
            for(int i=1; i<fanins.size(); i++) {
                res ^= fanins[i]->value;
            }
            res = !res;
            break;
        case INPUT:
            res = value;
            break;
        default:
            assert(false);
            break;
    }
    return res;
}

void Gate::cal_fault_info(int fd[2], int fpl[2]) {
    if(isPO) {
        fd[!value] = 1;
        fd[value] = 0;
        fpl[!value] = 0;
        fpl[value] = 0;
        return;
    }

    value ^= 1;

    std::vector<Gate*> affectd;
    std::unordered_map<Gate*, int> gate_level;
    std::priority_queue<Gate*, std::vector<Gate*>, 
        std::function<bool(Gate*, Gate*)>> q([](Gate* p1, Gate* p2) {
            return p1->id < p2->id;
    });

    for(Gate* out : fanouts) {
        if(out->value != out->cal_value()) {
            out->value ^= 1;
            gate_level[out] = 1;
            affectd.push_back(out);
            q.push(out);
        }
    }

    while(!q.empty()) {
        Gate* cur = q.top(); q.pop();
        for(Gate* out : cur->fanouts) {
            if(out->value != out->cal_value()) {
                out->value ^= 1;
                gate_level[out] = std::max(gate_level[out], gate_level[cur] + 1);
                affectd.push_back(out);
                q.push(out);
            }
        }
    }

    for(Gate* gate : affectd) {
        fault_detected[value] |= gate->fault_detected[gate->value];
        fault_propagated_len[value] = std::max(fpl[value], gate_level[gate] + gate->fault_propagated_len[gate->value]);

        gate->value ^= 1;
    }

    value ^= 1;
}