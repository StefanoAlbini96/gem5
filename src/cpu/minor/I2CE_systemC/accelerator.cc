

#include "params/I2CE_accelerator.hh"
#include "accelerator.hh"


void I2CE_accelerator::clock_thread()
{

    in_ptr_reg.write(0);
    en_prev_reg.write(false);
    en_reg.write(false);        // Probably it can be removed!
    idx_processed_cnt_reg.write(0);

    for(int lane=0; lane<N_LANES; lane++){
        packed_idx_reg[lane] = 0;
    }

    for(int stage=0; stage<EN_STAGES; stage++){
        pipeline_en[stage] = 0;
    }
    
    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){

            codebook_reg[learner][lane] = 0;
            inputs_reg[learner][lane] = 0;
        }
    }

    // for(int add_stage=0; add_stage<ADD_DRAIN_EN; add_stage++){
    //     add_drain_en_sig[add_stage].write(false);
    // }

    wait();

    while(1){


        en_prev_reg.write(en_prev_nxt.read());

        bool rising_edge_en = (en_input.read() && !en_prev_reg.read());

        // The EN is coming externally and we start counting
        // if(en_input.read() && !pipeline_en[0].read()){
        if(rising_edge_en && !pipeline_en[0].read()){
            pipeline_en[0].write(en_input.read());
            idx_processed_cnt_reg.write(0);
        } 
        else if (pipeline_en[0].read()){
            if(idx_processed_cnt_reg.read() >= (IDX_PER_LANE-1)){
                pipeline_en[0].write(false);
            } else {
                idx_processed_cnt_reg.write(idx_processed_cnt_reg.read() + 1);
            }
        }

        for(int stage=(EN_STAGES-1); stage>0; stage--){
            pipeline_en[stage].write(pipeline_en[stage-1].read());
        }



        // int nelems = input_fifo.size();
        // bool enough_data = (nelems >= (N_LEARNERS * N_LANES));

        // Packed indexes
        for(int lane=0; lane<N_LANES; lane++){
            packed_idx_reg[lane].write(packed_idx_nxt[lane]);
        }

        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                codebook_reg[learner][lane].write(codebook_nxt[learner][lane]);
            }
        }


        // Propagate the EN signal through the modules
        en_reg.write(en_nxt.read());
        // tbl_en_sig.write(tbl_en_nxt.read());
        mac_en_sig.write(mac_en_nxt.read());
        // red_en_sig.write(red_en_nxt.read());


        // EN for the TBL module
        // At the next cycle the MAC is EN and the activations are already in the register
        if(pipeline_en[0].read()){

            sum_reg.write(sum_nxt.read());

            for(int learner=0; learner<N_LEARNERS; learner++){
                for(int lane=0; lane<N_LANES; lane++){
                    inputs_reg[learner][lane] = inputs_nxt[learner][lane];
                }
            }

            in_ptr_reg.write(in_ptr_nxt.read());
        }

        wait();
    }
}



void I2CE_accelerator::comb_method()
{


    en_prev_nxt.write(en_input.read());


    // printf("Triggered\n");
    // Packed indexes
    for(int lane=0; lane<N_LANES; lane++){
        // std::cout << sc_time_stamp()
        //   << " packed_in = "
        //   << packed_in[lane].read()
        //   << std::endl;
        packed_idx_nxt[lane] = packed_in[lane];
    }



    uint16_t input_pointer = in_ptr_reg.read();

    // Inputs
    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){

            codebook_nxt[learner][lane] = codebook_in[learner][lane];

            // // Write 0 if the en is off so that the accumulation in the MAC module is correct
            // if(pipeline_en[0]){
                inputs_nxt[learner][lane] = inputs_in[learner][lane][input_pointer];
            // } else {
            //     inputs_nxt[learner][lane] = 0;
            // }
        }
    }


    bool can_update_in_ptr = (input_pointer < (ACT_BUF_SIZE - 1));
    // if(can_update_in_ptr){
        in_ptr_nxt = in_ptr_reg.read() + 1;
     // }



    // Update the EN signal
    en_nxt.write(en_reg.read());

    if(en_input.read()){
        en_nxt.write(true);
    }

    if(!can_update_in_ptr){
        en_nxt.write(false);
    }


    mac_en_nxt = tbl_en_sig.read();

}



// void I2CE_accelerator::comb_method()
// {


//     // printf("\n--- Comb method ---\n");

//     // tbl_en_nxt = en_reg.read();
//     mac_en_nxt = tbl_en_sig.read();
//     // red_en_nxt = mac_en_sig.read();

//     // Packed indexes
//     for(int lane=0; lane<N_LANES; lane++){
//         packed_idx_nxt[lane] = packed_in[lane];
//     }

//     uint16_t input_pointer = in_ptr_reg.read();

//     // Inputs
//     for(int learner=0; learner<N_LEARNERS; learner++){
//         for(int lane=0; lane<N_LANES; lane++){
//             codebook_nxt[learner][lane] = codebook_in[learner][lane];

//             inputs_nxt[learner][lane] = inputs_in[learner][lane][input_pointer];
//         }
//     }


//     en_nxt.write(en_reg.read());

//     if(en_input.read()){
//         en_nxt.write(true);
//     }

//     if (in_ptr_reg.read() >= (IDX_PER_LANE-1)){
//         en_nxt.write(false);
//         printf("EN = FALSE\n");
//     } else {
//         in_ptr_nxt = in_ptr_reg.read() + 1;
//     }




//     float test_result = 0.0;


//     for(int learn=0; learn<N_LEARNERS; learn++){
//         for(int lane=0; lane<N_LANES; lane++){
//             // printf("Input[%d] = %f\n", lane, inputs_reg[learn][lane].read());

//             test_result += inputs_reg[learn][lane].read();
//         }
//     }

//     sum_nxt.write(test_result);
// }



I2CE_accelerator *
gem5::I2CE_acceleratorParams::create() const
{
    I2CE_accelerator *acc = new I2CE_accelerator(name.c_str());
    return acc;
}