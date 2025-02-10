#include "circuit.h"
#include "simulator.h"
#include "paras.h"
#include "sat_atpg.h"
#include "checker.h"

using namespace atpg_ls;

int main(int argc, char *argv[]) {

    // 初始化命令行参数
    INIT_ARGS

    srand(OPT(seed));

    Circuit *circuit = new Circuit();

    Simulator* simulator = new Simulator();

    printf("parsing file %s ...\n", OPT(instance).c_str());
    circuit->parse_from_file(OPT(instance).c_str());
    circuit->insert_lines_for_stem();

    simulator->parse_from_file(OPT(instance).c_str());
    simulator->insert_lines_for_stem();
    
    circuit->init_topo_index();
    simulator->init_topo_index();

    
    circuit->init_avg_dist();

    circuit->print();

    init_final_checker();

    /**
     * D算法/电路 - 200000F = 时间优势好   (123) -> F -> (5,6)
     * SAT - 200000F * avg(N) (1011) (1010)
     * 
     * primary input 电路的输入
     * gate input 单个门的输入
     * 
     * (F0)
     * PIs assignment (10101010101010) -> (F0)
     * 
     * (10101010101010) inital sol
     * LOCAL + F0
     * 
     * (SAT + LS)
     **/
    sat_atpg_init(OPT(instance).c_str());
    
    // circuit->

    printf("building lut circuit ...\n");
    LUTCircuit *C = circuit->build_lut_circuit();
    C->simulator = simulator;

    printf("====== Circuit Statistics ====== \n");
    printf("PI:\t%ld\n", circuit->PIs.size());
    printf("PO:\t%ld\n", circuit->POs.size());
    printf("Gate:\t%ld\n", circuit->gates.size());
    printf("LUT:\t%ld\n", C->luts.size());
    printf("================================ \n");

    C->ls_main();

    return 0;
}
