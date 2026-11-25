#ifndef _GET_IDX_H_
#define _GET_IDX_H_


#ifdef GEM5
    #include "systemc/ext/systemc"
#else
    #include <systemc.h>
#endif


#ifdef __SYNTHESIS__
#include <ac_std_float.h>
#include <ac_sc.h>
#endif


#include <vector>
#include "definitions.hh"


using namespace sc_core;
using namespace sc_dt;

SC_MODULE(get_idx){


    ///////////////////
    // HW parameters //
    ///////////////////
    
    #ifdef GEM5
    int simd_lanes;
    #endif

    ///////////////////


    sc_in<bool>                                         clk;
    sc_in<bool>                                         rst;

    sc_in<bool>                                         in_en;

    sc_in<sc_dt::sc_uint<IDX_BIT_MAX>>                  bit_mask;
    sc_in<sc_dt::sc_uint<8>>                            idx_bits_in;
    sc_in<sc_dt::sc_uint<IDXS_PER_LANE_BITS>>           idxs_per_lane;
    // sc_in<sc_dt::sc_uint<IDXS_PER_LANE_BITS>>           processed_idxs;

    sc_in<sc_dt::sc_uint<IDXS_PER_LANE_BITS>>           cc_active;      // How many CC the EN signal should stay active

    // sc_in<sc_uint<LANE_IDX_BIT_MAX>>                    simd_lanes_in;


    // How many SIMD registers of activations do we need the indexes for
    // sc_in<sc_uint<32>>                                  n_simd_left;


    sc_signal<bool>                                     idx_en_reg, idx_en_nxt;
    sc_signal<bool>                                     idx_en_prev_reg, idx_en_prev_nxt;


    // Counts the CC the EN signal is active
    sc_signal<sc_dt::sc_uint<IDXS_PER_LANE_BITS>>       en_cc_counter_reg, en_cc_counter_nxt;



    // Selects which lane has to be unpacked
    sc_signal<sc_uint<LANE_IDX_BIT_MAX>>                sel_reg, sel_nxt;

    // Mask for the index
    sc_signal<sc_uint<IDX_BIT_MAX>>                     mask_reg;
    sc_signal<sc_uint<IDX_BIT_MAX>>                     bits_per_idx;

    sc_signal<sc_uint<SHAMT_BIT>>                       shamt_reg, shamt_nxt;

    // Acts as a predicate specifying the index of the last active SIMD lane
    // sc_signal<sc_uint<LANE_IDX_BIT_MAX>>                pred_last_active_lane_reg, pred_last_active_lane_nxt;


    // Vector register holding the unpacked indexes, output of this module
    #ifdef GEM5
    // SIMD register of different packed indexes
        SIMD_reg<sc_in<sc_dt::sc_uint<32>>>             packed_indexes;
        SIMD_reg<sc_signal<sc_uint<IDX_BIT_MAX>>>       idxs_reg, idxs_nxt;
        SIMD_reg<sc_out<sc_uint<IDX_BIT_MAX>>>          out;
    #else
        sc_in<uint32_t>                                 packed_indexes[N_LANES];
        sc_signal<sc_uint<IDX_BIT>>                     idxs_reg[N_LANES], idxs_nxt[N_LANES];
        sc_out<sc_uint<IDX_BIT_MAX>>                    out[N_LANES];
    #endif
    


    // sc_out<sc_uint<LANE_IDX_BIT_MAX>>       last_active_lane;

    sc_out<bool>                            en_out;




    #ifdef GEM5
    SC_CTOR(get_idx);
    get_idx(sc_core::sc_module_name name, int _simd_lanes)
    : simd_lanes(_simd_lanes)
    #else
    SC_CTOR(get_idx)
    #endif
    {

        #ifdef GEM5
            // Initialize multi dimensional sc_vectors
            idxs_reg.init(simd_lanes);
            idxs_nxt.init(simd_lanes);
            out.init(simd_lanes);
            packed_indexes.init(simd_lanes);
        #endif

        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        #ifdef GEM5
            int n_lanes = simd_lanes;
        #else
            int n_lanes = N_LANES;
        #endif

        SC_METHOD(comb_method);
        for(int lane=0; lane<n_lanes; lane++){
            sensitive << packed_indexes[lane];
            sensitive << idxs_reg[lane];
        }
        sensitive << shamt_reg;
        sensitive << sel_reg;
        sensitive << bits_per_idx;
        sensitive << in_en;
        sensitive << idx_en_reg;
        sensitive << idx_en_prev_reg;
        sensitive << idx_bits_in;
        sensitive << cc_active;
        sensitive << idxs_per_lane;
        // sensitive << simd_lanes_in;

    }



    void clock_thread();
    void comb_method();


};


#endif