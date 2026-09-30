#ifndef _GET_IDX_H_
#define _GET_IDX_H_


#include "systemc/ext/systemc"
#include <vector>

#include "definitions.hh"


using namespace sc_core;
using namespace sc_dt;

SC_MODULE(get_idx){

    sc_in<bool>                         clk;
    sc_in<bool>                         rst;

    sc_in<bool>                         get_idx_en;
    // sc_in<bool> ready;


    // sc_signal<bool>                     idx_en_sig, idx_en_nxt;

    // Selects which lane has to be unpacked
    sc_signal<sc_uint<LANE_IDX_BIT>>    sel_reg, sel_nxt;

    // Mask for the index
    sc_signal<sc_uint<IDX_BIT>>         mask_reg;

    // SIMD register of different packed indexes
    sc_in<sc_dt::sc_uint<32>>           packed_indexes[N_LANES];

    sc_signal<sc_uint<SHAMT_BIT>>       shamt_reg[N_LANES], shamt_nxt[N_LANES];


    // Vector register holding the unpacked indexes, output of this module
    sc_signal<sc_uint<IDX_BIT>>         idxs_reg[N_LANES], idxs_nxt[N_LANES];
    
    // sc_out<bool>                        en_out;
    sc_out<sc_uint<IDX_BIT>>            out[N_LANES];



    SC_CTOR(get_idx)
    {

        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(comb_method);
        for(int lane=0; lane<N_LANES; lane++){
            sensitive << packed_indexes[lane];
            sensitive << shamt_reg[lane];
        }
        sensitive << sel_reg;
        sensitive << get_idx_en;
    }



    void clock_thread();
    void comb_method();


};


#endif