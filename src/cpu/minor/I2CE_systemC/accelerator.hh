
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

    ///////////////////
    // HW parameters //
    ///////////////////
    
    int n_learners;
    int simd_lanes;

    ///////////////////

    sc_in<bool>                             clk;
    sc_in<bool>                             rst;

    sc_in<bool>                             rst_mac;
    sc_signal<bool>                         mac_res_reset_reg, mac_res_reset_nxt;
    sc_signal<bool>                         rst_mac_prev_reg, rst_mac_prev_nxt;
    // sc_signal<bool>                         mac_res_reset_prev_reg, mac_res_reset_prev_nxt;

    sc_in<bool>                             en_input;
    sc_signal<bool>                         en_prev_reg, en_prev_nxt;    // To detecet the rising edge --> start of compute loop
    // sc_signal<bool>                         en_reg, en_nxt;

    sc_signal<bool>                         tbl_en_sig, tbl_en_nxt;
    sc_signal<bool>                         mac_en_sig, mac_en_nxt;

    sc_signal<bool>                         mac_en_out_wire;

    sc_in<bool>                             red_trigger;
    sc_signal<bool>                         red_en_reg, red_en_nxt;
    sc_signal<bool>                         red_trigger_prev_reg, red_trigger_prev_nxt;


    sc_in<sc_dt::sc_uint<IDX_BIT_MAX>>      bit_mask;
    sc_in<sc_dt::sc_uint<8>>                bits_per_idx;

    // How many indexes are packed per lane
    sc_in<sc_uint<IDXS_PER_LANE_BITS>>                       idxs_per_lane_in;
    sc_signal<sc_uint<IDXS_PER_LANE_BITS>>                   idxs_per_lane;

    sc_in<bool>             en_trigger;
    sc_signal<bool>         en_reg, en_nxt;
    sc_signal<sc_uint<5>>   en_count_reg, en_count_nxt;
    sc_signal<bool>         en_trigger_prev_reg, en_trigger_prev_nxt;


    sc_signal<bool>                         pipeline_en[EN_STAGES];

    // sc_in<sc_dt::sc_uint<32>>               packed_in[N_LANES];
    SIMD_reg<sc_in<sc_dt::sc_uint<32>>>     packed_in;

    // sc_in<float>                            codebook_in[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_in<float>>       codebook_in;
    
    

    // sc_in<float>                            inputs_in[sN_LEARNERS][N_LANES][ACT_BUF_SIZE];
    multi_learner_SIMD_regs<sc_vector<sc_in<float>>>   inputs_in;
    sc_signal<sc_uint<log2_pow2(ACT_BUF_SIZE)>>                  in_ptr_reg, in_ptr_nxt; // this points at a specific input in the sequence
    
    // Counts the number of processed indexes so it knows when to stop the EN
    sc_signal<sc_uint<8>>                   idx_processed_cnt_reg, idx_processed_cnt_nxt; 

    // sc_in<float>                            res_tmp[N_LEARNERS];
    SIMD_reg<sc_in<float>>                  res_tmp;

    // sc_signal<float>                        res_from_mem_reg[N_LEARNERS];
    SIMD_reg<sc_signal<float>>              res_from_mem_reg;


    // Register for the packed indexes to be unpacked
    // sc_signal<sc_dt::sc_uint<32>>           packed_idx_reg[N_LANES], packed_idx_nxt[N_LANES];
    SIMD_reg<sc_signal<sc_dt::sc_uint<32>>>     packed_idx_reg, packed_idx_nxt;

    // Register for the codebook values
    // sc_signal<float>                        codebook_reg[N_LEARNERS][N_LANES], codebook_nxt[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_signal<float>>   codebook_reg, codebook_nxt;

    // Register for the input values
    // sc_signal<float>                        inputs_reg[N_LEARNERS][N_LANES], inputs_nxt[N_LEARNERS][N_LANES];
    multi_learner_SIMD_regs<sc_signal<float>>       inputs_reg, inputs_nxt;


    // sc_signal<sc_uint<IDX_BIT>>             res_getidx_mod_wire[N_LANES];           // wire to connect one output to the input of submodules
    // sc_signal<float>                        res_tbl_mod_wire[N_LEARNERS][N_LANES];  // wire to connect one output to the input of submodules
    // sc_signal<float>                        res_mac_mod_wire[N_LEARNERS][N_LANES];  // wire to connect one output to the input of submodules
    // sc_signal<float>                        res_red_mod_wire[N_LEARNERS];  // wire to connect one output to the input of submodules

    SIMD_reg<sc_signal<sc_uint<IDX_BIT_MAX>>>       res_getidx_mod_wire;
    multi_learner_SIMD_regs<sc_signal<float>>   res_tbl_mod_wire;
    multi_learner_SIMD_regs<sc_signal<float>>   res_mac_mod_wire;
    SIMD_reg<sc_signal<float>>                  res_red_mod_wire;



    // sc_out<float>                           red_out[N_LEARNERS];
    SIMD_reg<sc_out<float>>                 red_out;

    sc_signal<float>                        sum_reg, sum_nxt;    

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



    SC_CTOR(I2CE_accelerator);

    I2CE_accelerator(sc_core::sc_module_name name, int _n_learners, int _simd_lanes)
    : n_learners(_n_learners), simd_lanes(_simd_lanes)
    {

        inputs_in.init(n_learners);
        inputs_reg.init(n_learners);
        inputs_nxt.init(n_learners);

        codebook_in.init(n_learners);
        codebook_reg.init(n_learners);
        codebook_nxt.init(n_learners);
        
        packed_in.init(simd_lanes);
        packed_idx_reg.init(simd_lanes);
        packed_idx_nxt.init(simd_lanes);

        res_getidx_mod_wire.init(simd_lanes);
        res_tbl_mod_wire.init(n_learners);
        res_mac_mod_wire.init(n_learners);
        res_red_mod_wire.init(n_learners);

        red_out.init(n_learners);
        res_tmp.init(n_learners);
        res_from_mem_reg.init(n_learners);

        for(int learner=0; learner<n_learners; learner++){
            inputs_in[learner].init(simd_lanes);
            inputs_reg[learner].init(simd_lanes);
            inputs_nxt[learner].init(simd_lanes);

            codebook_in[learner].init(simd_lanes);
            codebook_reg[learner].init(simd_lanes);
            codebook_nxt[learner].init(simd_lanes);

            res_tbl_mod_wire[learner].init(simd_lanes);
            res_mac_mod_wire[learner].init(simd_lanes);

            for(int lane=0; lane<simd_lanes; lane++){
                inputs_in[learner][lane].init(ACT_BUF_SIZE);
            }
        }

    // SC_CTOR(I2CE_accelerator)
    // {

        getidx_mod = new get_idx("getidx_mod", simd_lanes);
        tbl_mod = new tbl("tbl_mod", n_learners, simd_lanes);
        mac_mod = new simd_mac("mac_mod", n_learners, simd_lanes);
        red_mod = new reduce_mod("red_mod", n_learners, simd_lanes);
        // st_mod = new store_mod("st_mod");


        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);

        // SC_METHOD(comb_method_red_en);
        // sensitive << mac_en_out_wire;

        SC_METHOD(comb_method);
        // sensitive << en_input;
        sensitive << en_reg;
        sensitive << en_prev_reg;

        sensitive << tbl_en_sig;
        sensitive << mac_en_sig;
        sensitive << in_ptr_reg;
        for(int lane=0; lane<simd_lanes; lane++){
            sensitive << packed_in[lane];
        }
        for(int learner=0; learner<n_learners; learner++){
            for(int lane=0; lane<simd_lanes; lane++){
                sensitive << codebook_in[learner][lane];

                for(int i=0; i<ACT_BUF_SIZE; i++){
                    sensitive << inputs_in[learner][lane][i];
                }
            }

            sensitive << res_tmp[learner];
        }


        SC_METHOD(trigger_mac_res_reset_comb_method);
        sensitive << rst_mac;
        sensitive << mac_res_reset_reg;


        SC_METHOD(trigger_reduce_comb_method);
        sensitive << red_trigger;
        sensitive << red_en_reg;


        SC_METHOD(trigger_en_comb_method);
        sensitive << en_trigger;
        sensitive << en_reg;
        sensitive << en_count_reg;


        // Bind the ports for the get_idx module
        getidx_mod->clk(clk);
        getidx_mod->rst(rst);
        // getidx_mod->rst(mac_res_reset_reg);

        // getidx_mod->get_idx_en(en_input);
        getidx_mod->in_en(en_reg);

        getidx_mod->bit_mask(bit_mask);
        getidx_mod->idx_bits_in(bits_per_idx);

        // getidx_mod->en_out(tbl_en_sig);
        for(int lane=0; lane<simd_lanes; lane++){
            getidx_mod->packed_indexes[lane](packed_idx_reg[lane]);
            getidx_mod->out[lane](res_getidx_mod_wire[lane]);
        }


        // Bind the ports for the tbl module
        tbl_mod->clk(clk);
        tbl_mod->rst(rst);
        // tbl_mod->tbl_en(tbl_en_sig);
        tbl_mod->tbl_en(pipeline_en[1]);
        for(int lane=0; lane<simd_lanes; lane++){
            tbl_mod->idxs[lane](res_getidx_mod_wire[lane]);
        }
        for(int learner=0; learner<n_learners; learner++){
            for(int lane=0; lane<simd_lanes; lane++){
                tbl_mod->codebook[learner][lane](codebook_reg[learner][lane]);
                tbl_mod->out[learner][lane](res_tbl_mod_wire[learner][lane]);
            }
        }


        // Bind the ports for the simd_mac module
        mac_mod->clk(clk);
        mac_mod->rst(mac_res_reset_reg);
        // mac_mod->clear_res_toggle(mac_res_reset_reg);
        mac_mod->mac_en(pipeline_en[2]);
        mac_mod->add_en(pipeline_en[3]);
        // mac_mod->en_out(red_en_sig);
        mac_mod->en_out(mac_en_out_wire);
        for(int learner=0; learner<n_learners; learner++){
            for(int lane=0; lane<simd_lanes; lane++){
                mac_mod->activation[learner][lane](inputs_reg[learner][lane]);
                mac_mod->weight[learner][lane](res_tbl_mod_wire[learner][lane]);
                mac_mod->out[learner][lane](res_mac_mod_wire[learner][lane]);
            }
        }


        // Bind the ports for the reduce module
        red_mod->clk(clk);
        red_mod->rst(rst);
        red_mod->red_en(red_en_reg);
        for(int learner=0; learner<n_learners; learner++){
            for(int lane=0; lane<simd_lanes; lane++){
                red_mod->mac_res[learner][lane](res_mac_mod_wire[learner][lane]);
            }
            red_mod->res_from_mem[learner](res_from_mem_reg[learner]);
            red_mod->out[learner](red_out[learner]);
        }


        // // Bind the ports for the store module
        // st_mod->clk(clk);
        // st_mod->rst(rst);
        // st_mod->st_en(red_en_reg);
        // for(int learner=0; learner<N_LEARNERS; learner++){
        //     st_mod->red_res[learner](res_red_mod_wire[learner]);
        //     st_mod->res_tmp_from_mem[learner](res_tmp[learner]);
        //     st_mod->out[learner](res_red_mod_wire[learner]);
        // }



    }

    
    void clock_thread();
    // void comb_method_red_en();
    void comb_method();
    void ld_res_mem();
    void trigger_reduce_comb_method();
    void trigger_mac_res_reset_comb_method();
    void trigger_en_comb_method();



    void end_of_elaboration() override
    {
        numAdditions
            .name(std::string(name()) + ".numAdditions")
            .desc("Number of additions performed by the `fadder` module.")
        ;
    }


};

#endif