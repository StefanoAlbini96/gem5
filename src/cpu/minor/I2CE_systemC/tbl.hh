#ifndef _TBL_H_
#define _TBL_H_

#include <systemc.h>
#include <vector>

#include "definitions.hh"



SC_MODULE(tbl){


    ///////////////////
    // HW parameters //
    ///////////////////
    
    int n_learners;
    int simd_lanes;

    ///////////////////


    sc_in<bool> clk;
    sc_in<bool> rst;

    // sc_in<bool> ready;
    sc_in<bool> tbl_en;




    // sc_signal<float>                    weights_reg[N_LEARNERS][N_LANES], weights_nxt[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_signal<float>>           weights_reg, weights_nxt;

    // sc_in<float>                             codebook[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_in<float>>               codebook;

    // sc_in<sc_uint<IDX_BIT>>             idxs[N_LANES];
    SIMD_reg<sc_in<sc_uint<IDX_BIT>>>                          idxs;

    // sc_out<float>                       out[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_out<float>>               out;


    SC_CTOR(tbl);

    tbl(sc_core::sc_module_name name, int _n_learners, int _simd_lanes)
    : n_learners(_n_learners), simd_lanes(_simd_lanes)
    {

        // Initialize multi dimensional sc_vectors

        weights_reg.init(n_learners);
        weights_nxt.init(n_learners);
        codebook.init(n_learners);
        idxs.init(simd_lanes);
        out.init(n_learners);

        for(int learn=0; learn<n_learners; learn++){
            weights_reg[learn].init(simd_lanes);
            weights_nxt[learn].init(simd_lanes);
            codebook[learn].init(simd_lanes);
            out[learn].init(simd_lanes);
        }



    // SC_CTOR(tbl)
    // {

        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(comb_method);
        for(int lane=0; lane<simd_lanes; lane++){
            sensitive << idxs[lane];
        }

        for(int learner=0; learner<n_learners; learner++){
            for(int lane=0; lane<simd_lanes; lane++){
                sensitive << codebook[learner][lane];
                sensitive << weights_reg[learner][lane];
            }
        }
    }


    void clock_thread();
    void comb_method();

};

#endif