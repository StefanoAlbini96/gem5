#ifndef _TBL_H_
#define _TBL_H_


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



SC_MODULE(tbl){


    ///////////////////
    // HW parameters //
    ///////////////////
    
    #ifdef GEM5
    int n_learners;
    int simd_lanes;
    #endif

    ///////////////////


    sc_in<bool>                                         clk;
    sc_in<bool>                                         rst;

    sc_in<bool>                                         tbl_en;


    #ifdef GEM5
        multi_learner_SIMD_regs<sc_signal<float_type>>      weights_reg, weights_nxt;
        multi_learner_SIMD_regs<sc_in<float_type>>          codebook;
        SIMD_reg<sc_in<sc_uint<IDX_BIT_MAX>>>               idxs;
        multi_learner_SIMD_regs<sc_out<float_type>>         out;
    #else

        sc_signal<float_type>                               weights_reg[N_LEARNERS][N_LANES], weights_nxt[N_LEARNERS][N_LANES];
        sc_in<float_type>                                   codebook[N_LEARNERS][N_LANES];
        sc_in<sc_uint<IDX_BIT_MAX>>                         idxs[N_LANES];
        sc_out<float_type>                                  out[N_LEARNERS][N_LANES];

    #endif



    #ifdef GEM5
    SC_CTOR(tbl);
    tbl(sc_core::sc_module_name name, int _n_learners, int _simd_lanes)
    : n_learners(_n_learners), simd_lanes(_simd_lanes)
    #else
    SC_CTOR(tbl)
    #endif
    {

        #ifdef GEM5
            // Initialize multi dimensional sc_vectors
            weights_reg.init(n_learners);
            weights_nxt.init(n_learners);
            codebook.init(n_learners);
            idxs.init(simd_lanes);
            out.init(n_learners);

            for(int learn=0; learn<n_learners; learn++){
                weights_reg[learn].init(simd_lanes);
                weights_nxt[learn].init(simd_lanes);
                codebook[learn].init(CB_SIZE_MAX);
                out[learn].init(simd_lanes);
            }
        #endif


        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);


        #ifdef GEM5
            int n_lanes = simd_lanes;
            int num_learners = n_learners;
        #else
            int n_lanes = N_LANES;
            int num_learners = N_LEARNERS;
        #endif

        SC_METHOD(comb_method);
        for(int lane=0; lane<n_lanes; lane++){
            sensitive << idxs[lane];
        }

        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){
                sensitive << weights_reg[learner][lane];
            }
            for(int lane=0; lane<CB_SIZE_MAX; lane++){
                sensitive << codebook[learner][lane];
            }
        }
    }


    void clock_thread();
    void comb_method();

};

#endif