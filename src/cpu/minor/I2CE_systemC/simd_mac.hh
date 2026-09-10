#ifndef _SIMD_MAC_H_
#define _SIMD_MAC_H_

#include <systemc.h>
#include <vector>

#include "definitions.hh"


SC_MODULE(simd_mac){

    sc_in<bool> clk;
    sc_in<bool> rst;
    
    // sc_in<bool> ready;
    sc_in<bool> mac_en; // Signals that the mac result should be registered
    sc_in<bool> add_en; // Enables the ADD DRAIN pipeline. This needs to have a delay so that the pipeline can be fully drained

    // sc_in<bool> clear_res_toggle;   // Whenever this is changed, the result registers are cleared

    sc_signal<bool>     mul_en_sig[MUL_EN];
    sc_signal<bool>     add_drain_en_sig[ADD_DRAIN_EN];

    sc_signal<bool>     add_en_sig, add_en_nxt;         // this enables the ADD (pipeline and add_res update, not the drain)
    sc_signal<uint8_t>  n_adds_cnt_reg, n_adds_cnt_nxt;


    sc_in<float>        activation[N_LEARNERS][N_LANES];
    sc_in<float>        weight[N_LEARNERS][N_LANES];

    sc_signal<float>    pipeline_mul[N_LEARNERS][N_LANES][MUL_STAGES];
    sc_signal<float>    mul_comb_res[N_LEARNERS][N_LANES];
    sc_signal<float>    mul_res_reg[N_LEARNERS][N_LANES], mul_res_nxt[N_LEARNERS][N_LANES];

    sc_signal<float>    pipeline_add[N_LEARNERS][N_LANES][ADD_STAGES];
    sc_signal<float>    add_comb_res[N_LEARNERS][N_LANES];
    sc_signal<float>    add_res_reg[N_LEARNERS][N_LANES], add_res_nxt[N_LEARNERS][N_LANES];     // For the result of the addition

    sc_signal<float>    pipeline_add_drain[N_LEARNERS][N_LANES][ADD_STAGES];
    sc_signal<float>    res_reg[N_LEARNERS][N_LANES], res_nxt[N_LEARNERS][N_LANES];             // For the final result, coming from the accumulation of the drain
    

    sc_out<bool>        en_out;     // output it to be able to synch with the subsequent module (as this one delays the EN)
    sc_out<float>       out[N_LEARNERS][N_LANES];


    SC_CTOR(simd_mac)
    {

        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(mult_comb_method);
        for(int en=0; en<MUL_EN; en++){
            sensitive << mul_en_sig[en];
        }
        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                sensitive << activation[learner][lane];
                sensitive << weight[learner][lane];
                sensitive << pipeline_mul[learner][lane][MUL_STAGES-1];
            }
        }

        SC_METHOD(add_comb_method);
        for(int en=0; en<ADD_DRAIN_EN; en++){
            sensitive << add_drain_en_sig[en];
        }
        // sensitive << clear_res_toggle;
        sensitive << add_en_sig;
        sensitive << n_adds_cnt_reg;
        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                sensitive << mul_res_reg[learner][lane];
                sensitive << add_res_reg[learner][lane];
                sensitive << pipeline_add[learner][lane][ADD_STAGES-1];
            }
        }

    }

    void clock_thread();
    void mult_comb_method();
    void add_comb_method();

};


#endif