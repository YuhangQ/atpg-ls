#include "simulator.h"

using namespace atpg_ls;

int Simulator::verify(LUTCircuit *lut_circuit, int** fault_detected) {

    // lut_circuit->check();

    // for(LUT* lut : lut_circuit->PIs) {
    //     printf("PI: %s = %d\n", lut->name, lut->value);
    // }

    int score;
    simulate(lut_circuit->PIs, score, fault_detected);

    for(LUT* lut : lut_circuit->luts) {
        assert(lut->value == lut->cal_value());
    }

    for(LUT* lut : lut_circuit->rtopo_luts) {
        lut->cal_fault_info(lut->fd, lut->fpl);
    }

    // lut_circuit->print();

    std::unordered_map<int, Gate*> id2gate;
    for(Gate* gate : gates) {
        id2gate[gate->id] = gate;
    }

    for(LUT* lut : lut_circuit->luts) {

        int input_var = 0;
        for(int i=0; i<lut->fanins.size(); i++) {
            LUT* l = lut->fanins[i];
            input_var |= l->value << i;
        }

        assert(input_var == lut->input_var);
        
        if(!lut->isPI) assert(lut->value_table[lut->input_var] == lut->value);

        // printf(">> LUT: %s\n", lut->name);

        // assert(lut->fd[0] == id2gate[lut->gate->id]->fault_detected[0]);
        // assert(lut->fd[1] == id2gate[lut->gate->id]->fault_detected[1]);
        // assert(lut->fpl[0] == id2gate[lut->gate->id]->fault_propagated_len[0]);
        // assert(lut->fpl[1] == id2gate[lut->gate->id]->fault_propagated_len[1]);

        for(int i=lut->fanins.size(); i<lut->fanins.size()+lut->inner_gates.size(); i++) {

            LUT::FaultInfo &info = lut->fault_table[i][(lut->input_var<<1)|lut->value];

            Gate* g = lut->inner_gates[i-lut->fanins.size()];

            int t_fd[2], t_fpl[2];
            t_fd[0] = info.fd[0] && lut->fd[!lut->value];
            t_fd[1] = info.fd[1] && lut->fd[!lut->value];
            t_fpl[0] = info.fpl[0] + info.fd[0] * lut->fpl[!lut->value];
            t_fpl[1] = info.fpl[1] + info.fd[1] * lut->fpl[!lut->value];

            Gate* sim_g = id2gate[g->id];
            assert(g->name == sim_g->name);

            // if(g->name == "new_n32_" || g->name == "new_n33_") {
                // printf("Gate: %s value: %d %d\n", g->name.c_str(), info.value, sim_g->value);
                // printf("l_fd[0]: %d, l_fd[1]: %d, l_fpl[0]: %d, l_fpl[1]: %d\n", lut->fd[0], lut->fd[1], lut->fpl[0], lut->fpl[1]);
                // printf("i_fd[0]: %d, i_fd[1]: %d, i_fpl[0]: %d, i_fpl[1]: %d\n", info.fd[0], info.fd[1], info.fpl[0], info.fpl[1]);
                // printf("t_fd[0]: %d, t_fd[1]: %d, t_fpl[0]: %d, t_fpl[1]: %d\n", t_fd[0], t_fd[1], t_fpl[0], t_fpl[1]);
                // printf("s_fd[0]: %d, s_fd[1]: %d, s_fpl[0]: %d, s_fpl[1]: %d\n\n", sim_g->fault_detected[0], sim_g->fault_detected[1], sim_g->fault_propagated_len[0], sim_g->fault_propagated_len[1]);
            // }

            assert(info.value == sim_g->value);
            assert(t_fd[0] == sim_g->fault_detected[0]);
            assert(t_fd[1] == sim_g->fault_detected[1]);
            // assert(t_fpl[0] == sim_g->fault_propagated_len[0]);
            // assert(t_fpl[1] == sim_g->fault_propagated_len[1]);
        }
    }

    return 1;
}

void Simulator::simulate(std::vector<LUT*> &inputs, int &score, int** fault_detected) {

    assert(inputs.size() == this->PIs.size());

    for(int i=0; i<inputs.size(); i++) {
        PIs[i]->value = inputs[i]->value;
    }

    for(auto gate : gates) {
        gate->value = gate->cal_value();
    }

    for(auto gate : rtopo_gates) {
        gate->fault_detected[0] = gate->cal_fault_detected(0);
        gate->fault_detected[1] = gate->cal_fault_detected(1);
        gate->fault_propagated_len[0] = gate->cal_propagate_len(0);
        gate->fault_propagated_len[1] = gate->cal_propagate_len(1);
    }

    score = 0;

    for(auto gate : gates) {
        if(gate->fault_detected[0] && !fault_detected[gate->id-1][0]) {
            score++;
        }
        if(gate->fault_detected[1] && !fault_detected[gate->id-1][1]) {
            score++;
        }
    }
}