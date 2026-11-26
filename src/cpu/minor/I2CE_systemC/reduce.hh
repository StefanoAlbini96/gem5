#ifndef _REDUCE_H_
#define _REDUCE_H_

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


SC_MODULE(reduce_mod){

    ///////////////////
    // HW parameters //
    ///////////////////
    
    #ifdef GEM5
    int n_learners;
    int simd_lanes;
    #endif

    ///////////////////




    sc_in<bool>                 clk;
    sc_in<bool>                 rst;
    
    sc_in<bool>                 red_en;



    #ifdef GEM5

        // Results loaded from memory and to be added to the new one
        SIMD_reg<sc_in<float_type>>                      res_from_mem;

        // Vector registers with the results to be reduced
        multi_learner_SIMD_regs<sc_in<float_type>>       mac_res;

        // Containes the reduced results
        SIMD_reg<sc_signal<float_type>>                  reduced_reg, reduced_nxt;

        SIMD_reg<sc_out<float_type>>                     out;

    #else
    
        sc_in<float_type>                                res_from_mem[N_LEARNERS];
        multi_learner_SIMD_regs<sc_in<float_type>>       mac_res[N_LEARNERS][N_LANES];
        sc_signal<float_type>                            reduced_reg[N_LEARNERS], reduced_nxt[N_LEARNERS];
        sc_out<float_type>                               out[N_LEARNERS];

    #endif




    #ifdef GEM5
    SC_CTOR(reduce_mod);

    reduce_mod(sc_core::sc_module_name name, int _n_learners, int _simd_lanes)
    : n_learners(_n_learners), simd_lanes(_simd_lanes)
    #else
    SC_CTOR(reduce_mod)
    #endif
    {

        #ifdef GEM5
            // Initialize multi dimensional sc_vectors
            res_from_mem.init(n_learners);

            mac_res.init(n_learners);
            
            reduced_reg.init(n_learners);
            reduced_nxt.init(n_learners);

            out.init(n_learners);


            for(int learn=0; learn<n_learners; learn++){
                mac_res[learn].init(simd_lanes);
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
        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){
                sensitive << mac_res[learner][lane];
            }
            sensitive << res_from_mem[learner];
            sensitive << reduced_reg[learner];
        }
    }

    void clock_thread();
    void comb_method();

};


#endif