
#ifndef __SYSTEMC_I2CE_ACCELERATOR_HH__
#define __SYSTEMC_I2CE_ACCELERATOR_HH__


#include "base/statistics.hh"
// // // #include "base/trace.hh"

#include "systemc/ext/systemc"


#include "definitions.hh"

#include "get_idx.hh"
#include "tbl.hh"
#include "simd_mac.hh"
#include "reduce.hh"
#include "store.hh"

using namespace sc_core;
using namespace sc_dt;



SC_MODULE(I2CE_accelerator){

    sc_in<bool>                             clk;
    sc_in<bool>                             rst;

    sc_in<bool>                             en_input;

    sc_in<bool>                             red_trigger;

    sc_signal<bool>                         en_prev_reg, en_prev_nxt;    // To detecet the rising edge --> start of compute loop
    sc_signal<bool>                         en_reg, en_nxt;

    sc_signal<bool>                         tbl_en_sig, tbl_en_nxt;
    sc_signal<bool>                         mac_en_sig, mac_en_nxt;

    sc_signal<bool>                         mac_en_out_wire;
    sc_signal<bool>                         red_en_reg, red_en_nxt;
    sc_signal<bool>                         red_trigger_prev_reg, red_trigger_prev_nxt;

    sc_signal<bool>                         pipeline_en[EN_STAGES];

    sc_in<sc_dt::sc_uint<32>>               packed_in[N_LANES];
    sc_in<float>                            codebook_in[N_LEARNERS][N_LANES];
    

    // std::deque<float>                       input_fifo;
    sc_in<float>                            inputs_in[N_LEARNERS][N_LANES][ACT_BUF_SIZE];
    sc_signal<sc_uint<log2_pow2(ACT_BUF_SIZE)>>                  in_ptr_reg, in_ptr_nxt; // this points at a specific input in the sequence
    
    // Counts the number of processed indexes so it knows when to stop the EN
    sc_signal<sc_uint<8>>                   idx_processed_cnt_reg, idx_processed_cnt_nxt; 

    sc_in<float>                            res_tmp[N_LEARNERS];

    // Register for the packed indexes to be unpacked
    sc_signal<sc_dt::sc_uint<32>>           packed_idx_reg[N_LANES], packed_idx_nxt[N_LANES];

    // Register for the codebook values
    sc_signal<float>                        codebook_reg[N_LEARNERS][N_LANES], codebook_nxt[N_LEARNERS][N_LANES];

    // Register for the input values
    sc_signal<float>                        inputs_reg[N_LEARNERS][N_LANES], inputs_nxt[N_LEARNERS][N_LANES];


    sc_signal<sc_uint<IDX_BIT>>             res_getidx_mod_wire[N_LANES];           // wire to connect one output to the input of submodules
    sc_signal<float>                        res_tbl_mod_wire[N_LEARNERS][N_LANES];  // wire to connect one output to the input of submodules
    sc_signal<float>                        res_mac_mod_wire[N_LEARNERS][N_LANES];  // wire to connect one output to the input of submodules
    sc_signal<float>                        res_red_mod_wire[N_LEARNERS];  // wire to connect one output to the input of submodules


    sc_out<float>                           red_out[N_LEARNERS];

    sc_signal<float>                        sum_reg, sum_nxt;    

    // sc_out<sc_uint<IDX_BIT>>  red_out[N_LEARNERS];


    // GET_IDX module //
    get_idx *getidx_mod;

    // TBL module //
    tbl *tbl_mod;

    // MAC module //
    simd_mac *mac_mod;

    // REDUCE module //
    reduce_mod *red_mod;

    // STORE module //
    store_mod *st_mod;




    gem5::statistics::Scalar numAdditions;

    SC_CTOR(I2CE_accelerator)
    {

        getidx_mod = new get_idx("getidx_mod");
        tbl_mod = new tbl("tbl_mod");
        mac_mod = new simd_mac("mac_mod");
        red_mod = new reduce_mod("red_mod");
        st_mod = new store_mod("st_mod");


        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        // SC_METHOD(comb_method_red_en);
        // sensitive << mac_en_out_wire;

        SC_METHOD(comb_method);
        sensitive << en_input;
        sensitive << en_prev_reg;
        sensitive << en_reg;
        sensitive << tbl_en_sig;
        sensitive << mac_en_sig;
        sensitive << in_ptr_reg;
        for(int lane=0; lane<N_LANES; lane++){
            sensitive << packed_in[lane];
        }
        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                sensitive << codebook_in[learner][lane];

                for(int i=0; i<ACT_BUF_SIZE; i++){
                    sensitive << inputs_in[learner][lane][i];
                }
            }
        }


        SC_METHOD(trigger_reduce_comb_method);
        sensitive << red_trigger;
        sensitive << red_en_reg;


        // Bind the ports for the get_idx module
        getidx_mod->clk(clk);
        getidx_mod->rst(rst);
        getidx_mod->get_idx_en(en_input);
        getidx_mod->en_out(tbl_en_sig);
        for(int lane=0; lane<N_LANES; lane++){
            getidx_mod->packed_indexes[lane](packed_idx_reg[lane]);
            getidx_mod->out[lane](res_getidx_mod_wire[lane]);
        }


        // Bind the ports for the tbl module
        tbl_mod->clk(clk);
        tbl_mod->rst(rst);
        // tbl_mod->tbl_en(tbl_en_sig);
        tbl_mod->tbl_en(pipeline_en[0]);
        for(int lane=0; lane<N_LANES; lane++){
            tbl_mod->idxs[lane](res_getidx_mod_wire[lane]);
        }
        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                tbl_mod->codebook[learner][lane](codebook_reg[learner][lane]);
                tbl_mod->out[learner][lane](res_tbl_mod_wire[learner][lane]);
            }
        }


        // Bind the ports for the simd_mac module
        mac_mod->clk(clk);
        mac_mod->rst(rst);
        // mac_mod->mac_en(mac_en_sig);
        // mac_mod->add_en(mac_en_sig);
        mac_mod->mac_en(pipeline_en[1]);
        mac_mod->add_en(pipeline_en[2]);
        // mac_mod->en_out(red_en_sig);
        mac_mod->en_out(mac_en_out_wire);
        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                mac_mod->activation[learner][lane](inputs_reg[learner][lane]);
                // mac_mod->activation[learner][lane](inputs_in[learner][lane]);
                mac_mod->weight[learner][lane](res_tbl_mod_wire[learner][lane]);
                mac_mod->out[learner][lane](res_mac_mod_wire[learner][lane]);
            }
        }


        // Bind the ports for the reduce module
        red_mod->clk(clk);
        red_mod->rst(rst);
        red_mod->red_en(red_en_reg);
        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                red_mod->mac_res[learner][lane](res_mac_mod_wire[learner][lane]);
            }
            red_mod->out[learner](res_red_mod_wire[learner]);
        }


        // Bind the ports for the store module
        st_mod->clk(clk);
        st_mod->rst(rst);
        st_mod->st_en(red_en_reg);
        for(int learner=0; learner<N_LEARNERS; learner++){
            st_mod->red_res[learner](res_red_mod_wire[learner]);
            st_mod->res_tmp_from_mem[learner](res_tmp[learner]);
            st_mod->out[learner](red_out[learner]);
        }



    }

    
    void clock_thread();
    // void comb_method_red_en();
    void comb_method();
    void trigger_reduce_comb_method();


    void end_of_elaboration() override
    {
        numAdditions
            .name(std::string(name()) + ".numAdditions")
            .desc("Number of additions performed by the `fadder` module.")
        ;
    }


};

#endif