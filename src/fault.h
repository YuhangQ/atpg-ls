#pragma once

#include "gate.h"

namespace atpg_ls {

struct TMP_FAULT {
    /* data */
    Gate *g;

    std::string name;
    int stuck_at;
    int is_stem;
    int is_PO;

    friend bool operator < (const TMP_FAULT &lhs, const TMP_FAULT &rhs) {
        if(lhs.name != rhs.name) {
            return lhs.name < rhs.name;
        }
        if(lhs.stuck_at != rhs.stuck_at) {
            return lhs.stuck_at < rhs.stuck_at;
        }
        if(lhs.is_stem != rhs.is_stem) {
            return lhs.is_stem < rhs.is_stem;
        }
        if(lhs.is_PO != rhs.is_PO) {
            return lhs.is_PO < rhs.is_PO;
        }
        return false;
    }
};

}