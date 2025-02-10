#include <unordered_set>

#include "checker.h"
#include "paras.h"

namespace atpg_ls {

Circuit *right_circuit;
Circuit *wrong_circuit;

int check_fault(const Pattern &p, const TMP_FAULT &f) {
    for(int i=0; i<p.input_vector.size(); i++) {
        right_circuit->PIs[i]->value = p.input_vector[i];
        wrong_circuit->PIs[i]->value = p.input_vector[i];
    }

    for(Gate* gate : right_circuit->gates) {
        gate->value = gate->cal_value();
    }

    for(Gate* gate : wrong_circuit->gates) {
        if(gate->name == f.g->name) {
            gate->value = f.stuck_at;
            continue;
        }
        gate->value = gate->cal_value();
    }

    for(int i=0; i<right_circuit->POs.size(); i++) {
        if(right_circuit->POs[i]->value != wrong_circuit->POs[i]->value) {
            return true;
        }
    }
    return false;
}

int final_check(const std::set<TMP_FAULT> &faults, const std::vector<Pattern> &patterns) {

    std::set<TMP_FAULT> detected_faults;

    for(auto &p : patterns) {
        for(auto &f : p.detected_faults) {
            
            printf("checking fault %s %d ... ", f.g->name.c_str(), f.stuck_at);

            assert(faults.count(f));
            assert(detected_faults.count(f) == 0);

            // check if F is detected by P

            // printf("pi size %d %d\n", right_circuit->PIs.size(), p.input_vector.size());

            assert(right_circuit->PIs.size() == p.input_vector.size());
            assert(wrong_circuit->PIs.size() == p.input_vector.size());

            int detected = check_fault(p, f);

            if(detected) {
                printf("detected\n");
                detected_faults.insert(f);
            } else {
                printf("undetected\n");
                assert(false);
            }
        }        
    }

    printf("final coverage: %d/%d(%.2f) pattern: %d\n", detected_faults.size(), faults.size(), (double)detected_faults.size()/faults.size(), patterns.size());

    return 1;
}

void init_final_checker() {
    right_circuit = new Circuit();
    wrong_circuit = new Circuit();

    right_circuit->parse_from_file(OPT(instance).c_str());
    wrong_circuit->parse_from_file(OPT(instance).c_str());

    right_circuit->insert_lines_for_stem();
    wrong_circuit->insert_lines_for_stem();

    right_circuit->init_topo_index();
    wrong_circuit->init_topo_index();
}

}

