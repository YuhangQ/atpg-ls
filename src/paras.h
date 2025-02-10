#ifndef _paras_hpp_INCLUDED
#define _paras_hpp_INCLUDED

#include <string>
#include <cstring>
#include <unordered_map>

//        name,       type,  short-name,must-need, default ,low, high, comments
#define PARAS \
    PARA( seed              , int    , '\0'  ,  false   , 17    , 0  , 100000000 , "max input numbers of LUT") \
    PARA( lut               , int    , '\0'  ,  false   , 8     , 0  , 30        , "max input numbers of LUT") \
    PARA( sp                , double , '\0'  ,  false   , 0.01  , 0  , 1         , "max input numbers of LUT") \
    PARA( brk_sp            , double , '\0'  ,  false   , 0.05  , 0  , 1         , "max input numbers of LUT") \
    PARA( t                 , int    , '\0'  ,  false   , 30    , 0  , 1000      , "max input numbers of LUT") \
    PARA( vsat_inc          , int    , '\0'  ,  false   , 1     , 0  , 100000    , "max input numbers of LUT") \
    PARA( vsat_max          , int    , '\0'  ,  false   , 10000 , 0  , 100000    , "max input numbers of LUT") \
    PARA( fw_inc            , int    , '\0'  ,  false   , 2     , 0  , 1000      , "max input numbers of LUT") \
    PARA( fw_max            , int    , '\0'  ,  false   , 10000 , 0  , 1000      , "max input numbers of LUT") \
    PARA( up_inc            , int    , '\0'  ,  false   , 5     , 0  , 1000      , "max input numbers of LUT") \
    PARA( up_max            , int    , '\0'  ,  false   , 10000 , 0  , 1000      , "max input numbers of LUT") \
    PARA( max_step_coeff    , double , '\0'  ,  false   , 10.0  , 1  , 1000      , "max input numbers of LUT")
//            name,   short-name, must-need, default, comments
#define STR_PARAS \
    STR_PARA( instance   , 'i'   ,  true    , "" , ".bench format instance")
    
struct paras 
{
#define PARA(N, T, S, M, D, L, H, C) \
    T N = D;
    PARAS 
#undef PARA

#define STR_PARA(N, S, M, D, C) \
    std::string N = D;
    STR_PARAS
#undef STR_PARA

void parse_args(int argc, char *argv[]);
void print_change();
};

#define INIT_ARGS __global_paras.parse_args(argc, argv);

extern paras __global_paras;

#define OPT(N) (__global_paras.N)

#endif