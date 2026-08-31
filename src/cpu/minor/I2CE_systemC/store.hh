#ifndef _STORE_H_
#define _STORE_H_

#include <systemc.h>
#include <vector>

#include "definitions.hh"


SC_MODULE(store_mod){


    sc_in<bool> clk;
    sc_in<bool> rst;
    
    sc_in<bool> st_en;

    sc_in<float> red_res[N_LEARNERS];

    sc_in<float> res_tmp_from_mem[N_LEARNERS];


    sc_out<float> out[N_LEARNERS];


    SC_CTOR(store_mod)
    {
        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(comb_method);
        for(int learner=0; learner<N_LEARNERS; learner++){
            sensitive << red_res[learner];
            sensitive << res_tmp_from_mem[learner];
        }
    }

    void clock_thread();
    void comb_method();

};


#endif