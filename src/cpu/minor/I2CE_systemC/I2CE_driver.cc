
#include "params/I2CE_driver.hh"
#include "sim/sim_exit.hh"
#include "cpu/minor/I2CE_systemC/I2CE_driver.hh"


namespace gem5
{

I2CE_driver::I2CE_driver(const gem5::I2CE_driverParams &params) :
    gem5::SimObject(params), 
    // gem5::ClockedObject(params), 
    accel(params.accel),
    clk("clk", sc_core::sc_time(0.2, sc_core::SC_NS))
    // clk("clk", sc_core::sc_time(clockPeriod(), sc_core::SC_PS))
    // clk(params.clk_domain)
{

    // I2CE_accelerator *acc = new I2CE_accelerator("accelerator");
    // accel = acc;
    delay = 1;

    accel->clk(clk);

    accel->en_input(en);
    accel->rst(rst);
    // accel->a(ab);
    // accel->b(ab);

    // Connect the packed indexes
    for(int lane=0; lane<N_LANES; lane++){
        accel->packed_in[lane](packed_indexes[lane]);
    }


    for(int learn=0; learn<N_LEARNERS; learn++){
        for(int lane=0; lane<N_LANES; lane++){
            accel->codebook_in[learn][lane](codebooks[learn][lane]);

            for(int i=0; i<IDX_PER_LANE; i++){
                accel->inputs_in[learn][lane][i](input_vect[learn][lane][i]);
            }
        }

        accel->red_out[learn](red_out[learn]);
        accel->res_tmp[learn](res_tmp[learn]);
    }



    rst.write(false);

}



void
I2CE_driver::startup()
{
    // Accelerator should not be fired (event handler --> drive()) when the simulation starts //
    // schedule(&event, gem5::curTick() + delay);

    printf("Starting...\n");


    printf("RST for the accelerator...\n");
    rst.write(true);
    rst.write(false);

    // WAVEFORMS creation
    tf = sc_core::sc_create_vcd_trace_file("i2ce_wave");
    sc_core::sc_trace(tf, clk, "clk");
    sc_core::sc_trace(tf, rst, "rst");
    
    sc_core::sc_trace(tf, en, "en");
    sc_core::sc_trace(tf, accel->en_reg, "accel.en_reg");
    sc_core::sc_trace(tf, accel->tbl_mod->tbl_en, "accel.tbl_mod.tbl_mod_EN");
    sc_core::sc_trace(tf, accel->mac_mod->mac_en, "accel.mac_mod_EN");


    sc_core::sc_trace(tf, accel->in_ptr_nxt, "accel.IN_pointer_nxt");
    sc_core::sc_trace(tf, accel->in_ptr_reg, "accel.IN_pointer_reg");

    sc_core::sc_trace(tf, accel->getidx_mod->sel_nxt, "accel.GET_IDX.sel_nxt");
    sc_core::sc_trace(tf, accel->getidx_mod->sel_reg, "accel.GET_IDX.sel_reg");

    sc_core::sc_trace(tf, accel->getidx_mod->idx_en_nxt, "accel.GET_IDX.idx_en_nxt");
    sc_core::sc_trace(tf, accel->getidx_mod->idx_en_sig, "accel.GET_IDX.idx_en_sig");


    for(int stage=0; stage<EN_STAGES; stage++){
        sc_core::sc_trace(tf, accel->pipeline_en[stage], "accel.pipeline_en[" + std::to_string(stage) + "]");
    }

    for(int mul_stage=0; mul_stage<(MUL_EN); mul_stage++){
        sc_core::sc_trace(tf, accel->mac_mod->mul_en_sig[mul_stage], "accel.mac_mod.mul_en_sig[" + std::to_string(mul_stage) + "]");
    }
    for(int add_stage=0; add_stage<(ADD_DRAIN_EN); add_stage++){
        sc_core::sc_trace(tf, accel->mac_mod->add_drain_en_sig[add_stage], "accel.mac_mod.add_drain_en_sig[" + std::to_string(add_stage) + "]");
    }


    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){

            for(int i=0; i<IDX_PER_LANE; i++){
                sc_core::sc_trace(tf, input_vect[learner][lane][i], "input_vect[" + std::to_string(learner) + "][" + std::to_string(lane) + "][" + std::to_string(i) + "]");
                sc_core::sc_trace(tf, accel->inputs_in[learner][lane][i], "accel.inputs_in[" + std::to_string(learner) + "][" + std::to_string(lane) + "][" + std::to_string(i) + "]");            
            }

            sc_core::sc_trace(tf, codebooks[learner][lane], "codebooks[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");
            sc_core::sc_trace(tf, accel->codebook_reg[learner][lane], "accel.codebook_reg[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");

            sc_core::sc_trace(tf, accel->res_tbl_mod_wire[learner][lane], "accel.res_tbl_mod_wire[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");

            sc_core::sc_trace(tf, accel->inputs_nxt[learner][lane], "accel.inputs_nxt[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");
            sc_core::sc_trace(tf, accel->inputs_reg[learner][lane], "accel.inputs_reg[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");



            // TBL module
            sc_core::sc_trace(tf, accel->tbl_mod->codebook[learner][lane], "accel.tbl_mod.codebook[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");
            sc_core::sc_trace(tf, accel->tbl_mod->weights_nxt[learner][lane], "accel.tbl_mod.weights_nxt[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");
            sc_core::sc_trace(tf, accel->tbl_mod->weights_reg[learner][lane], "accel.tbl_mod.weights_reg[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");


            for(int stage=0; stage<MUL_STAGES; stage++){
                sc_core::sc_trace(tf, accel->mac_mod->pipeline_mul[learner][lane][stage], "accel.mac_mod.pipeline_mul[" + std::to_string(learner) + "][" + std::to_string(lane) + "][" + std::to_string(stage) + "]");
            }
            for(int stage=0; stage<ADD_STAGES; stage++){
                sc_core::sc_trace(tf, accel->mac_mod->pipeline_add[learner][lane][stage], "accel.mac_mod.pipeline_add[" + std::to_string(learner) + "][" + std::to_string(lane) + "][" + std::to_string(stage) + "]");
                sc_core::sc_trace(tf, accel->mac_mod->pipeline_add_drain[learner][lane][stage], "accel.mac_mod.pipeline_add_drain[" + std::to_string(learner) + "][" + std::to_string(lane) + "][" + std::to_string(stage) + "]");
            }
            sc_core::sc_trace(tf, accel->mac_mod->add_res_reg[learner][lane], "accel.mac_mod.add_res_reg[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");

            sc_core::sc_trace(tf, accel->mac_mod->activation[learner][lane], "accel.mac_mod.activation[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");
            sc_core::sc_trace(tf, accel->mac_mod->weight[learner][lane], "accel.mac_mod.weight[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");

            sc_core::sc_trace(tf, accel->mac_mod->mul_res_reg[learner][lane], "accel.mac_mod.mul_res_reg[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");
            sc_core::sc_trace(tf, accel->mac_mod->add_res_reg[learner][lane], "accel.mac_mod.add_res_reg[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");
            sc_core::sc_trace(tf, accel->mac_mod->res_reg[learner][lane], "accel.mac_mod.res_reg[" + std::to_string(learner) + "][" + std::to_string(lane) + "]");
        }


        sc_core::sc_trace(tf, accel->red_mod->reduced_reg[learner], "accel.red_mod.reduced_reg[" + std::to_string(learner) + "]");

        sc_core::sc_trace(tf, accel->red_out[learner], "accel.RED_OUT[" + std::to_string(learner) + "]");

        sc_core::sc_trace(tf, accel->res_tmp[learner], "accel.RES_TMP[" + std::to_string(learner) + "]");

        sc_core::sc_trace(tf, accel->res_red_mod_wire[learner], "accel.res_red_mod_wire[" + std::to_string(learner) + "]");
    
    }

    for(int lane=0; lane<N_LANES; lane++){
        sc_core::sc_trace(tf, accel->packed_in[lane], "accel.packed_in[" + std::to_string(lane) + "]");
        sc_core::sc_trace(tf, accel->packed_idx_nxt[lane], "accel.packed_idx_nxt[" + std::to_string(lane) + "]");
        sc_core::sc_trace(tf, accel->packed_idx_reg[lane], "accel.packed_idx_reg[" + std::to_string(lane) + "]");
        sc_core::sc_trace(tf, accel->getidx_mod->packed_indexes[lane], "accel.GET_IDX.packed_indexes[" + std::to_string(lane) + "]");
        sc_core::sc_trace(tf, accel->getidx_mod->shamt_reg[lane], "accel.GET_IDX.shamt_reg[" + std::to_string(lane) + "]");

        sc_core::sc_trace(tf, accel->res_getidx_mod_wire[lane], "accel.res_getidx_mod_wire[" + std::to_string(lane) + "]");



        // TBL module
        sc_core::sc_trace(tf, accel->tbl_mod->idxs[lane], "accel.tbl_mod.idxs[" + std::to_string(lane) + "]");
    }
}




void
I2CE_driver::push(float new_in)
{

//     // printf("[DRIVER] New IN received = %f\n", new_in);

//     // input_fifo.push_back(new_in);
//     accel->input_fifo.push_back(new_in);
    
//     int nelems = accel->input_fifo.size();
//     en.write(true);
//     // printf("[push()] FIFO nelems = %d\n", nelems);

}


void
I2CE_driver::compute_enable()
{
    printf("Enabling the accelerator\n");
    en.write(true);
}


void
I2CE_driver::compute_disable()
{
    printf("Disabling the accelerator\n");
    en.write(false);
}

// void
// I2CE_driver::compute()
// {
//     printf("--- Compute ---\n");

//     int nelems = input_fifo.size();
//     printf("[compute()] FIFO nelems = %d\n", nelems);

//     bool enough_data = (nelems >= (N_LEARNERS * N_LANES));
//     bool mac_ready = accel->mac_en_sig.read();

//     if(enough_data){
//         en.write(true);
//         if(mac_ready){

//             printf("Enough data --> EN\n");

//             for(int learn=0; learn<N_LEARNERS; learn++){
//                 for(int i=0; i<N_LANES; i++){
//                     // input_vect[i].write(input_fifo.read());
//                     input_vect[learn][i].write(input_fifo.front());
//                     input_fifo.pop_front();

//                     nelems = input_fifo.size();
//                     printf("[compute() pop_front()] FIFO nelems = %d\n", nelems);
//                 }
//             }
//         }
//     } else {
//         en.write(false);
//         printf("Few data --> !EN\n");
//     }


// }





void
I2CE_driver::drive()
{
    
    // std::cout << sc_core::sc_is_running() << "\n";
    // printf("Driving with value 10.0...\n");

    // en.write(true);

    // ab.write(10.0);


    // test();
}


void I2CE_driver::stall()
{
    printf("Stalling....\n");
    en.write(false);

    // Close VCD trace file
    // sc_core::sc_close_vcd_trace_file(tf);
}


void I2CE_driver::enable()
{
    printf("Enabling....\n");
    // en.write(true);
}



void I2CE_driver::ld_inputs(int learner, int lane, int in_idx, float input_val)
{
    // printf("LD inputs --> %d %d %d --> %f\n", learner, lane, in_idx, input_val);

    input_vect[learner][lane][in_idx] = input_val;
}


void I2CE_driver::ld_codebooks(int learner, int lane, float cb_word)
{

    // printf("\nLoading codebooks[%d][%d] = %f\n", learner, lane, cb_word);
    codebooks[learner][lane].write(cb_word);
}


void I2CE_driver::load_packed_idxs(int lane, uint32_t idx_word)
{

    // printf("\nLoading packed indexes = %d\n", idx_word);
    packed_indexes[lane].write(idx_word);
}


void I2CE_driver::push_tmp_res(int learner_id, float val)
{
    // printf("Driver --> pushing tmp res = %d | %f\n", learner_id, val);
    res_tmp[learner_id].write(val);
}

float I2CE_driver::get_out_value(int learner_id)
{
    // printf("Driver --> getting out value = %d --> %f\n", learner_id, red_out[learner_id]);
    return red_out[learner_id];
}




I2CE_driver::~I2CE_driver()
{
    sc_core::sc_close_vcd_trace_file(tf);
    printf("Closing VCD\n");
}



} // namespace gem5