#include "sat_atpg.h"

#include "circuit_graph.h"
#include "iscas89_parser.h"
#include "circuit_to_cnf.h"
#include "fault_cnf.h"
#include "fault_manager.h"
#include "sat/sat_solver.h"
#include "solver_proxy.h"

#include "util/log.h"
#include "util/timer.h"

#include <fstream>
#include <algorithm>
#include <iostream>
#include <map>

CircuitGraph *graph;
Iscas89Parser *parser;
FaultManager *fault_manager;
FaultCnfMaker *fault_cnf_maker;
std::unique_ptr<SatSolver> solver;
ProxyCnf *proxy;

std::map<TMP_FAULT, Fault> fault_map;

bool sat_atpg(const TMP_FAULT &fal, std::vector<int> &input_vector) {

    assert(fault_map.count(fal) != 0);

    Fault f = fault_map[fal];

    fault_cnf_maker->make_fault(f, *proxy);

    SatSolver::SolveStatus status = solver->solve_prepared();

    for (Line* l : graph->get_inputs()) {
		int8_t val = solver->get_value(line_to_literal(l->id));
        input_vector.push_back(val == -1 ? 0 : 1);
	}

    if(status == SatSolver::Sat) {
        return true;
    } else if(status == SatSolver::Unsat) {
        return false;
    } else {
        assert(false);
        printf(">> UNKNOWN\n");
    }
}

void sat_atpg_init(const char* file) {

    graph = new CircuitGraph();
    parser = new Iscas89Parser();

    std::ifstream ifs(file);
    if (!ifs.good()) {
		std::cout << "can't open file" << file;
		exit(-1);
	}
    
	if (!parser->parse(ifs, *graph)) {
		std::cout << "can't parse file" << file;
		exit(-1);
	}

    solver = SolverFactory::make_solver();

    if (!solver) {
		std::cout << "No SAT solver, can't run";
		exit(-1);
	}

    fault_manager = new FaultManager(*graph);

    fault_cnf_maker = new FaultCnfMaker(*graph);
    fault_cnf_maker->set_threshold_ratio(0.6);
    

    proxy = new ProxyCnf(*solver);

    int sz = 0;

    while(fault_manager->has_faults_left()) {
        Fault f = fault_manager->next_fault();

        sz++;

        if(f.is_stem || (f.line->source == nullptr && f.line->destinations.size() == 1)) {
            
            printf("Fault: %s stuck-at: %d is_stem: %d is_po: %d dest: %d\n", f.line->name.c_str(), f.stuck_at, f.is_stem, f.is_primary_output, f.line->destinations.size());
            TMP_FAULT tf = TMP_FAULT{nullptr, f.line->name, f.stuck_at, f.is_stem, f.is_primary_output};
            assert(fault_map.count(tf) == 0);
            fault_map[tf] = f;

        } else {

            std::string name;

            if(f.is_primary_output) {
                name = f.line->name + "_line_PO";
            } else {
                name = f.line->name + "_line_" + f.connection.gate->get_output()->name;
            }
            
            printf("Fault: %s stuck-at: %d is_stem: %d is_po: %d dest: %d\n", name.c_str(), f.stuck_at, f.is_stem, f.is_primary_output, f.line->destinations.size());

            TMP_FAULT tf = TMP_FAULT{nullptr, name, f.stuck_at, f.is_stem, f.is_primary_output};
            assert(fault_map.count(tf) == 0);
            fault_map[tf] = f;
        }

        // printf(">> Fault: %s stuck-at: %d is_stem: %d is_po: %d\n", f.line->name.c_str(), f.stuck_at, f.is_stem, f.is_primary_output);
    }

    printf("tg-pro fault-size: %d table-size: %d\n", sz, fault_map.size());

    assert(sz == fault_map.size());
    // exit(0);
}