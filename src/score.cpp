#include "lut.h"
#include "circuit.h"

using namespace atpg_ls;

void LUT::cal_score() {
    
    score = 0;
    score_value_unsatisfied_cost = 0;
    score_fault_detected_weight = 0;
    score_fault_propagated_weight = 0;
    score_fault_update_cost = 0;

    // value = !value;
    this->flip_value();

    if(uptag) {
        score_fault_update_cost -= up_cost;
    }
    
    // value sat score
    
    if(!isPI) {
        if(vsat) score_value_unsatisfied_cost += vunat_cost;
        else score_value_unsatisfied_cost -= vunat_cost;
        vsat = !vsat;
    }

    for(LUT* out : fanouts) {
        if(!out->vsat && out->cal_value() == out->value) {
            score_value_unsatisfied_cost -= out->vunat_cost;
            out->vsat = 1;
        }
        if(out->vsat && out->cal_value() != out->value){
            score_value_unsatisfied_cost += out->vunat_cost;
            out->vsat = 0;
        }
    }

    int old_fd[2], old_fpl[2];
    old_fd[0] = fd[0];
    old_fd[1] = fd[1];
    old_fpl[0] = fpl[0];
    old_fpl[1] = fpl[1];

    // fault detected score

    cal_fault_info(fd, fpl);

    // printf("fd: %d %d %d %d\n", fd[0], fd[1], old_fd[0], old_fd[1]);

    for(auto&[out, id] : fanouts_with_id) {
        uint32_t in1 = out->input_var, in2 = out->input_var;
        in1 ^= (1 << id);

        in1 = (in1 << 1) | out->value;
        in2 = (in2 << 1) | out->value;

        for(int i=out->fanins.size(); i<out->fanins.size()+out->inner_gates.size(); i++) {
            Gate* g = out->inner_gates[i-out->fanins.size()];
            
            int t_fd1[2], t_fpl1[2];
            int t_fd2[2], t_fpl2[2];
            
            t_fd1[0] = out->fault_table[i][in1].fd[0] && out->fd[!out->value];
            t_fd1[1] = out->fault_table[i][in1].fd[1] && out->fd[!out->value];

            t_fd2[0] = out->fault_table[i][in2].fd[0] && out->fd[!out->value];
            t_fd2[1] = out->fault_table[i][in2].fd[1] && out->fd[!out->value];

            t_fpl1[0] = out->fault_table[i][in1].fpl[0] + out->fault_table[i][in1].fd[0] * out->fpl[!out->value];
            t_fpl1[1] = out->fault_table[i][in1].fpl[1] + out->fault_table[i][in1].fd[1] * out->fpl[!out->value];

            t_fpl2[0] = out->fault_table[i][in2].fpl[0] + out->fault_table[i][in2].fd[0] * out->fpl[!out->value];
            t_fpl2[1] = out->fault_table[i][in2].fpl[1] + out->fault_table[i][in2].fd[1] * out->fpl[!out->value];

            score_fault_detected_weight += (t_fd2[0] - t_fd1[0]) * g->fault_detected_weight[0];
            score_fault_detected_weight += (t_fd2[1] - t_fd1[1]) * g->fault_detected_weight[1];

            if(!g->isPO) {
                score_fault_propagated_weight += (double)(t_fpl2[0] - t_fpl1[0]) / g->avg_dist * g->fault_detected_weight[0];
                score_fault_propagated_weight += (double)(t_fpl2[1] - t_fpl1[1]) / g->avg_dist * g->fault_detected_weight[1];
            }
        }
    }

    int in1 = this->input_var, in2 = this->input_var;
    in1 = (in1 << 1) | !value;
    in2 = (in2 << 1) | value;

    for(int i=fanins.size(); i<fanins.size()+inner_gates.size(); i++) {
        Gate* g = inner_gates[i-fanins.size()];

        int t_fd1[2], t_fpl1[2];
        int t_fd2[2], t_fpl2[2];
        
        t_fd1[0] = fault_table[i][in1].fd[0] && old_fd[value];
        t_fd1[1] = fault_table[i][in1].fd[1] && old_fd[value];

        t_fd2[0] = fault_table[i][in2].fd[0] && fd[!value];
        t_fd2[1] = fault_table[i][in2].fd[1] && fd[!value];

        t_fpl1[0] = fault_table[i][in1].fpl[0] + fault_table[i][in1].fd[0] * old_fpl[value];
        t_fpl1[1] = fault_table[i][in1].fpl[1] + fault_table[i][in1].fd[1] * old_fpl[value];

        t_fpl2[0] = fault_table[i][in2].fpl[0] + fault_table[i][in2].fd[0] * fpl[!value];
        t_fpl2[1] = fault_table[i][in2].fpl[1] + fault_table[i][in2].fd[1] * fpl[!value];

        score_fault_detected_weight += (t_fd2[0] - t_fd1[0]) * g->fault_detected_weight[0];
        score_fault_detected_weight += (t_fd2[1] - t_fd1[1]) * g->fault_detected_weight[1];

        if(!g->isPO) {
            score_fault_propagated_weight += (double)(t_fpl2[0] - t_fpl1[0]) / g->avg_dist * g->fault_detected_weight[0];
            score_fault_propagated_weight += (double)(t_fpl2[1] - t_fpl1[1]) / g->avg_dist * g->fault_detected_weight[1];
        }
    }

    // update cost score
    for(LUT* r : reigon) {
        int t_fd[2], t_fpl[2];
        r->cal_fault_info(t_fd, t_fpl);
        if(t_fd[0] == r->fd[0] && t_fd[1] == r->fd[1] && t_fpl[0] == r->fpl[0] && t_fpl[1] == r->fpl[1]) continue;
        if(!r->uptag) {
            score_fault_update_cost += r->up_cost;
        }
    }
    
    this->flip_value();
    if(!isPI) vsat = !vsat;

    fd[0] = old_fd[0];
    fd[1] = old_fd[1];
    fpl[0] = old_fpl[0];
    fpl[1] = old_fpl[1];

    for(LUT* out : fanouts) {
        out->vsat = ( out->cal_value() == out->value );
    }

    score = - score_value_unsatisfied_cost + score_fault_detected_weight + score_fault_propagated_weight - score_fault_update_cost;
}