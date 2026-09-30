#ifndef _SIMD_MAC_H_
#define _SIMD_MAC_H_

#include <systemc.h>
#include <vector>

#include "definitions.hh"


SC_MODULE(simd_mac){


    ///////////////////
    // HW parameters //
    ///////////////////
    
    int n_learners;
    int simd_lanes;

    ///////////////////



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


    // sc_in<float>        activation[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_in<float>>       activation;

    // sc_in<float>        weight[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_in<float>>       weight;

    // sc_signal<float>    pipeline_mul[N_LEARNERS][N_LANES][MUL_STAGES];
    // sc_signal<float>    mul_comb_res[N_LEARNERS][N_LANES];
    // sc_signal<float>    mul_res_reg[N_LEARNERS][N_LANES], mul_res_nxt[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_vector<sc_signal<float>>>        pipeline_mul;
    multi_learner_SIMD_regs<sc_signal<float>>                   mul_comb_res;
    multi_learner_SIMD_regs<sc_signal<float>>                   mul_res_reg, mul_res_nxt;


    // sc_signal<float>    pipeline_add[N_LEARNERS][N_LANES][ADD_STAGES];
    // sc_signal<float>    add_comb_res[N_LEARNERS][N_LANES];
    // sc_signal<float>    add_res_reg[N_LEARNERS][N_LANES], add_res_nxt[N_LEARNERS][N_LANES];     // For the result of the addition
    multi_learner_SIMD_regs<sc_vector<sc_signal<float>>>        pipeline_add;
    multi_learner_SIMD_regs<sc_signal<float>>                   add_comb_res;
    multi_learner_SIMD_regs<sc_signal<float>>                   add_res_reg, add_res_nxt;



    // sc_signal<float>    pipeline_add_drain[N_LEARNERS][N_LANES][ADD_STAGES];
    // sc_signal<float>    res_reg[N_LEARNERS][N_LANES], res_nxt[N_LEARNERS][N_LANES];             // For the final result, coming from the accumulation of the drain
    multi_learner_SIMD_regs<sc_vector<sc_signal<float>>>        pipeline_add_drain;
    multi_learner_SIMD_regs<sc_signal<float>>                   res_reg, res_nxt;
    

    sc_out<bool>        en_out;     // output it to be able to synch with the subsequent module (as this one delays the EN)

    // sc_out<float>       out[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_out<float>>      out;


    SC_CTOR(simd_mac);

    simd_mac(sc_core::sc_module_name name, int _n_learners, int _simd_lanes)
    : n_learners(_n_learners), simd_lanes(_simd_lanes)
    {

        // Initialize multi dimensional sc_vectors

        activation.init(n_learners);
        weight.init(n_learners);
        out.init(n_learners);

        pipeline_mul.init(n_learners);
        mul_comb_res.init(n_learners);
        mul_res_reg.init(n_learners);
        mul_res_nxt.init(n_learners);

        pipeline_add.init(n_learners);
        add_comb_res.init(n_learners);
        add_res_reg.init(n_learners);
        add_res_nxt.init(n_learners);

        pipeline_add_drain.init(n_learners);

        res_reg.init(n_learners);
        res_nxt.init(n_learners);

        for(int learn=0; learn<n_learners; learn++){
            activation[learn].init(simd_lanes);
            weight[learn].init(simd_lanes);
            out[learn].init(simd_lanes);


            pipeline_mul[learn].init(simd_lanes);
            mul_comb_res[learn].init(simd_lanes);
            mul_res_reg[learn].init(simd_lanes);
            mul_res_nxt[learn].init(simd_lanes);


            pipeline_add[learn].init(simd_lanes);
            add_comb_res[learn].init(simd_lanes);
            add_res_reg[learn].init(simd_lanes);
            add_res_nxt[learn].init(simd_lanes);

            pipeline_add_drain[learn].init(simd_lanes);

            res_reg[learn].init(simd_lanes);
            res_nxt[learn].init(simd_lanes);

            for(int lane=0; lane<simd_lanes; lane++){
                pipeline_mul[learn][lane].init(MUL_STAGES);

                pipeline_add[learn][lane].init(ADD_STAGES);

                pipeline_add_drain[learn][lane].init(ADD_STAGES);
            }

        }
        

        
        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        SC_METHOD(mult_comb_method);
        for(int en=0; en<MUL_EN; en++){
            sensitive << mul_en_sig[en];
        }
        for(int learner=0; learner<n_learners; learner++){
            for(int lane=0; lane<simd_lanes; lane++){
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
        for(int learner=0; learner<n_learners; learner++){
            for(int lane=0; lane<simd_lanes; lane++){
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