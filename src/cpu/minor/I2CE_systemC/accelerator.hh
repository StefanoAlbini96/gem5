
#ifndef __SYSTEMC_I2CE_ACCELERATOR_HH__
#define __SYSTEMC_I2CE_ACCELERATOR_HH__


#include "definitions.hh"


#ifdef GEM5
    #include "systemc/ext/systemc"
    #include "base/statistics.hh"
#else
    #include <systemc.h>
#endif


#ifdef __SYNTHESIS__
#include <ac_std_float.h>
#include <ac_sc.h>
#endif

#include "get_idx.hh"
#include "tbl.hh"
#include "simd_mac.hh"
#include "reduce.hh"

using namespace sc_core;
using namespace sc_dt;



SC_MODULE(I2CE_accelerator){

    ///////////////////
    // HW parameters //
    ///////////////////
    
    #ifdef GEM5
    int n_learners;
    int simd_lanes;
    #endif

    ///////////////////

    sc_in<bool>                             clk;
    sc_in<bool>                             rst;

    sc_signal<sc_uint<LANE_IDX_BIT_MAX>>             simd_lanes_reg;

    // Resets the get_idx module whenever new packed indexes are loaded
    sc_in<bool>                             rst_getidx;     
    sc_signal<bool>                         getidx_reset_reg, getidx_reset_nxt;
    sc_signal<bool>                         rst_getidx_prev_reg, rst_getidx_prev_nxt;

    sc_in<bool>                             rst_mac;
    sc_signal<bool>                         mac_res_reset_reg, mac_res_reset_nxt;
    sc_signal<bool>                         rst_mac_prev_reg, rst_mac_prev_nxt;

    // To reset the pointer to the buffer element
    sc_in<bool>                             rst_in_ptr;    
    sc_signal<bool>                         rst_in_ptr_prev_reg, rst_in_ptr_prev_nxt;    
    sc_signal<bool>                         can_reset_in_ptr_reg, can_reset_in_ptr_nxt;

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
    sc_in<sc_uint<IDXS_PER_LANE_BITS>>          idxs_per_lane_in;
    sc_signal<sc_uint<IDXS_PER_LANE_BITS>>      idxs_per_lane;
    sc_signal<sc_uint<32>>                      idxs_per_simd_reg, idxs_per_simd_nxt;                   // How many indexes I can pack in a SIMD vector
    sc_signal<sc_uint<IDXS_PER_LANE_BITS>>      cc_active_getidx_reg, cc_active_getidx_nxt;             // How many CC get_idx module should stay active

    // sc_signal<sc_uint<LANE_IDX_BIT_MAX>>                    last_active_lane;

    sc_in<bool>             en_trigger;
    sc_signal<bool>         en_reg, en_nxt;
    sc_signal<sc_uint<5>>   en_count_reg, en_count_nxt;
    sc_signal<bool>         en_trigger_prev_reg, en_trigger_prev_nxt;


    sc_signal<bool>                         pipeline_en[EN_STAGES];

    // EN signal that follows the internal EN signal of GET_IDX module, with a 1 CC delay (only the start, then end follows precisely)
    // Used to stop the input pointer when there are no more indexes to be extracted
    sc_signal<bool>                         in_ptr_en_reg, in_ptr_en_nxt;     
    sc_signal<bool>                         get_idx_en_interal;     // Internal EN of GET_IDX module
    
    

    sc_in<sc_uint<32>>                                  n_simd_loads;
    sc_signal<sc_uint<32>>                              n_simd_act_left_reg, n_simd_act_left_nxt; // How many activations (SIMD) are left to be processed

    sc_signal<sc_uint<log2_pow2(ACT_BUF_SIZE)>>         in_ptr_reg, in_ptr_nxt; // this points at a specific input in the sequence
    sc_signal<sc_uint<log2_pow2(ACT_BUF_SIZE)>>         in_ptr_next_reg, in_ptr_next_nxt; 
    sc_signal<bool>                                     processed_all_buf_reg, processed_all_buf_nxt;

    
    // Counts the number of processed indexes so it knows when to stop the EN
    sc_signal<sc_uint<IDXS_PER_LANE_BITS>>                   idx_processed_cnt_reg, idx_processed_cnt_nxt; 


    #ifdef GEM5

        SIMD_reg<sc_in<sc_dt::sc_uint<32>>>                         packed_in;
        multi_learner_SIMD_regs<sc_in<float_type>>                  codebook_in;
        multi_learner_SIMD_regs<sc_vector<sc_in<float_type>>>       inputs_in;
        SIMD_reg<sc_in<float_type>>                                 res_tmp;

        SIMD_reg<sc_signal<float_type>>                             res_from_mem_reg;

        // Register for the packed indexes to be unpacked
        SIMD_reg<sc_signal<sc_dt::sc_uint<32>>>                     packed_idx_reg, packed_idx_nxt;

        // Register for the codebook values
        multi_learner_SIMD_regs<sc_signal<float_type>>              codebook_reg, codebook_nxt;

        // Register for the input values
        multi_learner_SIMD_regs<sc_signal<float_type>>              inputs_reg, inputs_nxt;

        SIMD_reg<sc_signal<sc_uint<IDX_BIT_MAX>>>                   res_getidx_mod_wire;
        multi_learner_SIMD_regs<sc_signal<float_type>>              res_tbl_mod_wire;
        multi_learner_SIMD_regs<sc_signal<float_type>>              res_mac_mod_wire;
        SIMD_reg<sc_signal<float_type>>                             res_red_mod_wire;

        SIMD_reg<sc_out<float_type>>                                red_out;

    #else

        sc_in<sc_dt::sc_uint<32>>                   packed_in[N_LANES];
        sc_in<float_type>                            codebook_in[N_LEARNERS][N_LANES];
        sc_in<float_type>                            inputs_in[N_LEARNERS][N_LANES][ACT_BUF_SIZE];
        sc_in<float_type>                            res_tmp[N_LEARNERS];

        sc_signal<float_type>                        res_from_mem_reg[N_LEARNERS];
        sc_signal<sc_dt::sc_uint<32>>               packed_idx_reg[N_LANES], packed_idx_nxt[N_LANES];
        sc_signal<float_type>                        codebook_reg[N_LEARNERS][N_LANES], codebook_nxt[N_LEARNERS][N_LANES];
        sc_signal<float_type>                        inputs_reg[N_LEARNERS][N_LANES], inputs_nxt[N_LEARNERS][N_LANES];

        sc_signal<sc_uint<IDX_BIT>>                 res_getidx_mod_wire[N_LANES];           
        sc_signal<float_type>                        res_tbl_mod_wire[N_LEARNERS][N_LANES];  
        sc_signal<float_type>                        res_mac_mod_wire[N_LEARNERS][N_LANES];  
        sc_signal<float_type>                        res_red_mod_wire[N_LEARNERS];  

        sc_out<float_type>                           red_out[N_LEARNERS];

    #endif


    sc_signal<float_type>                        sum_reg, sum_nxt;    

    // GET_IDX module //
    get_idx *getidx_mod;

    // TBL module //
    tbl *tbl_mod;

    // MAC module //
    simd_mac *mac_mod;

    // REDUCE module //
    reduce_mod *red_mod;


    #ifdef GEM5
    gem5::statistics::Scalar numAdditions;
    #endif



    #ifdef GEM5
    SC_CTOR(I2CE_accelerator);

    I2CE_accelerator(sc_core::sc_module_name name, int _n_learners, int _simd_lanes)
    : n_learners(_n_learners), simd_lanes(_simd_lanes)
    #else
    SC_CTOR(I2CE_accelerator)
    #endif
    {

        #ifdef GEM5
            printf("Accel.n_learners = %d\n", n_learners);
            printf("Accel.simd_lanes = %d\n", simd_lanes);

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

                codebook_in[learner].init(CB_SIZE_MAX);
                codebook_reg[learner].init(CB_SIZE_MAX);
                codebook_nxt[learner].init(CB_SIZE_MAX);

                res_tbl_mod_wire[learner].init(simd_lanes);
                res_mac_mod_wire[learner].init(simd_lanes);

                for(int lane=0; lane<simd_lanes; lane++){
                    inputs_in[learner][lane].init(ACT_BUF_SIZE);
                }
            }
        #endif


        getidx_mod = new get_idx("getidx_mod", simd_lanes);
        tbl_mod = new tbl("tbl_mod", n_learners, simd_lanes);
        mac_mod = new simd_mac("mac_mod", n_learners, simd_lanes);
        red_mod = new reduce_mod("red_mod", n_learners, simd_lanes);


        SC_CTHREAD(clock_thread, clk.pos());
        async_reset_signal_is(rst, true);


        #ifdef GEM5
            int n_lanes = simd_lanes;
            int num_learners = n_learners;
        #else
            int n_lanes = N_LANES;
            int num_learners = N_LEARNERS;
        #endif

        SC_METHOD(comb_method);
        sensitive << en_reg;
        sensitive << en_prev_reg;

        sensitive << tbl_en_sig;
        sensitive << mac_en_sig;
        sensitive << in_ptr_reg;
        sensitive << in_ptr_next_reg;

        for(int lane=0; lane<n_lanes; lane++){
            sensitive << packed_in[lane];
        }

        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){
                for(int i=0; i<ACT_BUF_SIZE; i++){
                    sensitive << inputs_in[learner][lane][i];
                }
            }
            for(int lane=0; lane<CB_SIZE_MAX; lane++){
                sensitive << codebook_in[learner][lane];
            }
            sensitive << res_tmp[learner];
        }


        SC_METHOD(update_idx_per_simd);
        sensitive << idxs_per_lane_in;

        SC_METHOD(trigger_getidx_reset_comb_method);
        sensitive << rst_getidx;
        sensitive << getidx_reset_reg;

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


        SC_METHOD(delay_idx_en);
        sensitive << get_idx_en_interal;
        sensitive << processed_all_buf_reg;
        sensitive << n_simd_act_left_reg;
        
        SC_METHOD(reset_in_ptr);
        sensitive << rst_in_ptr;
        sensitive << rst_in_ptr_prev_reg;


        // Bind the ports for the get_idx module
        getidx_mod->clk(clk);
        getidx_mod->rst(getidx_reset_reg);
        getidx_mod->in_en(en_reg);
        getidx_mod->cc_active(cc_active_getidx_reg);
        getidx_mod->bit_mask(bit_mask);
        getidx_mod->idx_bits_in(bits_per_idx);
        getidx_mod->idxs_per_lane(idxs_per_lane);
        getidx_mod->en_out(get_idx_en_interal);
        for(int lane=0; lane<n_lanes; lane++){
            getidx_mod->packed_indexes[lane](packed_idx_reg[lane]);
            getidx_mod->out[lane](res_getidx_mod_wire[lane]);
        }


        // Bind the ports for the tbl module
        tbl_mod->clk(clk);
        tbl_mod->rst(rst);
        tbl_mod->tbl_en(pipeline_en[1]);
        for(int lane=0; lane<n_lanes; lane++){
            tbl_mod->idxs[lane](res_getidx_mod_wire[lane]);
        }
        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){
                tbl_mod->out[learner][lane](res_tbl_mod_wire[learner][lane]);
            }

            for(int lane=0; lane<CB_SIZE_MAX; lane++){
                tbl_mod->codebook[learner][lane](codebook_reg[learner][lane]);
            }
        }


        // Bind the ports for the simd_mac module
        mac_mod->clk(clk);
        mac_mod->rst(mac_res_reset_reg);
        mac_mod->mac_en(pipeline_en[2]);
        mac_mod->add_en(pipeline_en[3]);
        mac_mod->en_out(mac_en_out_wire);
        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){
                mac_mod->activation[learner][lane](inputs_reg[learner][lane]);
                mac_mod->weight[learner][lane](res_tbl_mod_wire[learner][lane]);
                mac_mod->out[learner][lane](res_mac_mod_wire[learner][lane]);
            }
        }


        // Bind the ports for the reduce module
        red_mod->clk(clk);
        red_mod->rst(rst);
        red_mod->red_en(red_en_reg);
        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){
                red_mod->mac_res[learner][lane](res_mac_mod_wire[learner][lane]);
            }
            red_mod->res_from_mem[learner](res_from_mem_reg[learner]);
            red_mod->out[learner](red_out[learner]);
        }

    }

    
    void clock_thread();
    void comb_method();
    void update_idx_per_simd();
    void ld_res_mem();
    void trigger_reduce_comb_method();
    void trigger_getidx_reset_comb_method();
    void trigger_mac_res_reset_comb_method();
    void trigger_en_comb_method();
    void delay_idx_en();
    void reset_in_ptr();



    #ifdef GEM5
    void end_of_elaboration() override
    {
        numAdditions
            .name(std::string(name()) + ".numAdditions")
            .desc("Number of additions performed by the `fadder` module.")
        ;
    }
    #endif


};

#endif