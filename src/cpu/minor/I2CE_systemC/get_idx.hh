#ifndef _GET_IDX_H_
#define _GET_IDX_H_


#include "systemc/ext/systemc"
#include <vector>

#include "definitions.hh"


using namespace sc_core;
using namespace sc_dt;

SC_MODULE(get_idx){


    ///////////////////
    // HW parameters //
    ///////////////////
    
    int simd_lanes;

    ///////////////////


    sc_in<bool>                         clk;
    sc_in<bool>                         rst;

    sc_in<bool>                         in_en;
    // sc_in<bool> ready;

    sc_in<sc_dt::sc_uint<IDX_BIT_MAX>>      bit_mask;
    sc_in<sc_dt::sc_uint<8>>                idx_bits_in;

    sc_signal<bool>                     idx_en_reg, idx_en_nxt;
    sc_signal<bool>                     idx_en_prev_reg, idx_en_prev_nxt;

    // Selects which lane has to be unpacked
    sc_signal<sc_uint<LANE_IDX_BIT_MAX>>    sel_reg, sel_nxt;

    // Mask for the index
    sc_signal<sc_uint<IDX_BIT_MAX>>         mask_reg;
    sc_signal<sc_uint<8>>                   bits_per_idx;                  

    // SIMD register of different packed indexes
    // sc_in<sc_dt::sc_uint<32>>           packed_indexes[N_LANES];
    SIMD_reg<sc_in<sc_dt::sc_uint<32>>>     packed_indexes;

    // sc_signal<sc_uint<SHAMT_BIT>>       shamt_reg[N_LANES], shamt_nxt[N_LANES];
    // SIMD_reg<sc_signal<sc_uint<SHAMT_BIT>>>     shamt_reg, shamt_nxt;
    sc_signal<sc_uint<SHAMT_BIT>>           shamt_reg, shamt_nxt;


    // Vector register holding the unpacked indexes, output of this module
    // sc_signal<sc_uint<IDX_BIT>>         idxs_reg[N_LANES], idxs_nxt[N_LANES];
    SIMD_reg<sc_signal<sc_uint<IDX_BIT_MAX>>>       idxs_reg, idxs_nxt;
    
    // sc_out<sc_uint<IDX_BIT>>            out[N_LANES];
    SIMD_reg<sc_out<sc_uint<IDX_BIT_MAX>>>  out;



    SC_CTOR(get_idx);

    get_idx(sc_core::sc_module_name name, int _simd_lanes)
    : simd_lanes(_simd_lanes)
    {

        // Initialize multi dimensional sc_vectors
        // shamt_reg.init(simd_lanes);
        // shamt_nxt.init(simd_lanes);
        idxs_reg.init(simd_lanes);
        idxs_nxt.init(simd_lanes);
        out.init(simd_lanes);



        packed_indexes.init(simd_lanes);

        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(comb_method);
        for(int lane=0; lane<simd_lanes; lane++){
            sensitive << packed_indexes[lane];
            // sensitive << shamt_reg[lane];
            sensitive << idxs_reg[lane];
        }
        sensitive << shamt_reg;
        sensitive << sel_reg;
        sensitive << bits_per_idx;
        sensitive << in_en;
        sensitive << idx_en_reg;
        sensitive << idx_en_prev_reg;
        sensitive << idx_bits_in;


        // SC_METHOD(comb_method_set_shamt);
        // sensitive << bits_per_idx;
        // sensitive << in_en;
        // sensitive << idx_en_reg;
        // sensitive << idx_en_prev_reg;
        // sensitive << idx_bits_in;
        // for(int lane=0; lane<simd_lanes; lane++){
        //     sensitive << idxs_reg[lane];
        // }


    }



    void clock_thread();
    void comb_method();
    // void comb_method_set_shamt();


};


#endif