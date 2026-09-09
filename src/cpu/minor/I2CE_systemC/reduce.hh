#ifndef _REDUCE_H_
#define _REDUCE_H_

#include <systemc.h>
#include <vector>

#include "definitions.hh"


SC_MODULE(reduce_mod){


    sc_in<bool> clk;
    sc_in<bool> rst;
    
    sc_in<bool> red_en;

    // Vector registers with the results to be reduced
    sc_in<float> mac_res[N_LEARNERS][N_LANES];

    // Results loaded from memory and to be added to the new one
    sc_in<float> res_from_mem[N_LEARNERS];

    // Containes the reduced results
    sc_signal<float> reduced_reg[N_LEARNERS], reduced_nxt[N_LEARNERS];

    sc_out<float> out[N_LEARNERS];


    SC_CTOR(reduce_mod)
    {
        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(comb_method);
        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
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