#ifndef _TBL_H_
#define _TBL_H_

#include <systemc.h>
#include <vector>

#include "definitions.hh"



SC_MODULE(tbl){

    sc_in<bool> clk;
    sc_in<bool> rst;

    // sc_in<bool> ready;
    sc_in<bool> tbl_en;




    sc_signal<float>                    weights_reg[N_LEARNERS][N_LANES], weights_nxt[N_LEARNERS][N_LANES];

    sc_in<float>                        codebook[N_LEARNERS][N_LANES];
    sc_in<sc_uint<IDX_BIT>>             idxs[N_LANES];
    sc_out<float>                       out[N_LEARNERS][N_LANES];

    SC_CTOR(tbl)
    {

        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(comb_method);
        for(int lane=0; lane<N_LANES; lane++){
            sensitive << idxs[lane];
        }

        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                sensitive << codebook[learner][lane];
                sensitive << weights_reg[learner][lane];
            }
        }
    }


    void clock_thread();
    void comb_method();

};

#endif