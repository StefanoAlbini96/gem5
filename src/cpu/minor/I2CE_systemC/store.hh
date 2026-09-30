#ifndef _STORE_H_
#define _STORE_H_

#include <systemc.h>
#include <vector>

#include "definitions.hh"


SC_MODULE(store_mod){


    sc_in<bool> clk;
    sc_in<bool> rst;
    
    sc_in<bool> st_en;

    sc_in<float> red_res[4];

    sc_in<float> res_tmp_from_mem[4];


    sc_out<float> out[4];


    SC_CTOR(store_mod)
    {
        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(comb_method);
        for(int learner=0; learner<4; learner++){
            sensitive << red_res[learner];
            sensitive << res_tmp_from_mem[learner];
        }
    }

    void clock_thread();
    void comb_method();

};


#endif