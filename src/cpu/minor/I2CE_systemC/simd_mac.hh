#ifndef _SIMD_MAC_H_
#define _SIMD_MAC_H_


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



SC_MODULE(simd_mac){


    ///////////////////
    // HW parameters //
    ///////////////////
    
    #ifdef GEM5
    int n_learners;
    int simd_lanes;
    #endif

    ///////////////////



    sc_in<bool>             clk;
    sc_in<bool>             rst;
    
    sc_in<bool>             mac_en; // Signals that the mac result should be registered
    sc_in<bool>             add_en; // Enables the ADD DRAIN pipeline. This needs to have a delay so that the pipeline can be fully drained

    sc_signal<bool>         mul_en_sig[MUL_EN];
    sc_signal<bool>         add_drain_en_sig[ADD_DRAIN_EN];

    sc_signal<bool>         add_en_sig, add_en_nxt;         // this enables the ADD (pipeline and add_res update, not the drain)
    sc_signal<uint8_t>      n_adds_cnt_reg, n_adds_cnt_nxt;

    sc_out<bool>            en_out;     // output it to be able to synch with the subsequent module (as this one delays the EN)


    #ifdef GEM5

        multi_learner_SIMD_regs<sc_in<float_type>>                       activation;
        multi_learner_SIMD_regs<sc_in<float_type>>                       weight;

        multi_learner_SIMD_regs<sc_vector<sc_signal<float_type>>>        pipeline_mul;
        multi_learner_SIMD_regs<sc_signal<float_type>>                   mul_comb_res;
        multi_learner_SIMD_regs<sc_signal<float_type>>                   mul_res_reg, mul_res_nxt;

        multi_learner_SIMD_regs<sc_vector<sc_signal<float_type>>>        pipeline_add;
        multi_learner_SIMD_regs<sc_signal<float_type>>                   add_comb_res;
        multi_learner_SIMD_regs<sc_signal<float_type>>                   add_res_reg, add_res_nxt;

        multi_learner_SIMD_regs<sc_vector<sc_signal<float_type>>>        pipeline_add_drain;
        multi_learner_SIMD_regs<sc_signal<float_type>>                   res_reg, res_nxt;

        multi_learner_SIMD_regs<sc_out<float_type>>                      out;

    #else

        sc_in<float_type>        activation[N_LEARNERS][N_LANES];

        sc_in<float_type>        weight[N_LEARNERS][N_LANES];

        sc_signal<float_type>    pipeline_mul[N_LEARNERS][N_LANES][MUL_STAGES];
        sc_signal<float_type>    mul_comb_res[N_LEARNERS][N_LANES];
        sc_signal<float_type>    mul_res_reg[N_LEARNERS][N_LANES], mul_res_nxt[N_LEARNERS][N_LANES];


        sc_signal<float_type>    pipeline_add[N_LEARNERS][N_LANES][ADD_STAGES];
        sc_signal<float_type>    add_comb_res[N_LEARNERS][N_LANES];
        sc_signal<float_type>    add_res_reg[N_LEARNERS][N_LANES], add_res_nxt[N_LEARNERS][N_LANES];     // For the result of the addition



        sc_signal<float_type>    pipeline_add_drain[N_LEARNERS][N_LANES][ADD_STAGES];
        sc_signal<float_type>    res_reg[N_LEARNERS][N_LANES], res_nxt[N_LEARNERS][N_LANES];             // For the final result, coming from the accumulation of the drain

        sc_out<float_type>       out[N_LEARNERS][N_LANES];


    #endif




    #ifdef GEM5
    SC_CTOR(simd_mac);

    simd_mac(sc_core::sc_module_name name, int _n_learners, int _simd_lanes)
    : n_learners(_n_learners), simd_lanes(_simd_lanes)

    #else

    SC_CTOR(simd_mac)
    
    #endif
    {

        #ifdef GEM5
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


        SC_METHOD(mult_comb_method);
        for(int en=0; en<MUL_EN; en++){
            sensitive << mul_en_sig[en];
        }
        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){
                sensitive << activation[learner][lane];
                sensitive << weight[learner][lane];
                sensitive << pipeline_mul[learner][lane][MUL_STAGES-1];
            }
        }

        SC_METHOD(add_comb_method);
        for(int en=0; en<ADD_DRAIN_EN; en++){
            sensitive << add_drain_en_sig[en];
        }
        sensitive << add_en_sig;
        sensitive << n_adds_cnt_reg;
        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){
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