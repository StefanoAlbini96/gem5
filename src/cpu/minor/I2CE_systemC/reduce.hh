#ifndef _REDUCE_H_
#define _REDUCE_H_

#include <systemc.h>
#include <vector>

#include "definitions.hh"


SC_MODULE(reduce_mod){

    ///////////////////
    // HW parameters //
    ///////////////////
    
    int n_learners;
    int simd_lanes;

    ///////////////////




    sc_in<bool> clk;
    sc_in<bool> rst;
    
    sc_in<bool> red_en;

    // Vector registers with the results to be reduced
    multi_learner_SIMD_regs<sc_in<float>>       mac_res;

    // Results loaded from memory and to be added to the new one
    // sc_in<float> res_from_mem[N_LEARNERS];
    SIMD_reg<sc_in<float>>      res_from_mem;

    // Containes the reduced results
    // sc_signal<float> reduced_reg[N_LEARNERS], reduced_nxt[N_LEARNERS];
    SIMD_reg<sc_signal<float>>      reduced_reg, reduced_nxt;

    // sc_out<float> out[N_LEARNERS];
    SIMD_reg<sc_out<float>>     out;


    SC_CTOR(reduce_mod);

    reduce_mod(sc_core::sc_module_name name, int _n_learners, int _simd_lanes)
    : n_learners(_n_learners), simd_lanes(_simd_lanes)
    {

        // Initialize multi dimensional sc_vectors
        res_from_mem.init(n_learners);

        mac_res.init(n_learners);
        
        reduced_reg.init(n_learners);
        reduced_nxt.init(n_learners);

        out.init(n_learners);


        for(int learn=0; learn<n_learners; learn++){
            mac_res[learn].init(simd_lanes);
        }



        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(comb_method);
        for(int learner=0; learner<n_learners; learner++){
            for(int lane=0; lane<simd_lanes; lane++){
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