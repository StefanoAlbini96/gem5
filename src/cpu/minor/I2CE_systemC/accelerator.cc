

#include "params/I2CE_accelerator.hh"
#include "accelerator.hh"


void I2CE_accelerator::clock_thread()
{

    for(int lane=0; lane<N_LANES; lane++){
        packed_idx_reg[lane] = 0;
    }
    
    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){
            
            inputs_reg[learner][lane] = 0;
            codebook_reg[learner][lane] = 0;
        }
    }

    wait();

    while(1){
        // wait(a.value_changed_event());
        // wait();
        // std::cout << "t = " << sc_core::sc_time_stamp() << std::endl;


        en.write(en_input.read());

        int nelems = input_fifo.size();
        bool enough_data = (nelems >= (N_LEARNERS * N_LANES));


        // Packed indexes
        for(int lane=0; lane<N_LANES; lane++){
            packed_idx_reg[lane].write(packed_idx_nxt[lane]);
        }

        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){
                codebook_reg[learner][lane].write(codebook_nxt[learner][lane]);
            }
        }



        if(en.read()){

            // printf("ACCEL ENABLED\n");
            // float res = a.read() + b.read();
            // numAdditions++;

            // printf("Res = %f + %f = %f\n", a.read(), b.read(), res);

            // printf("ENOUGH DATA? %d\n", nelems);

            // Propagate the EN signal through the modules
            tbl_en_sig.write(tbl_en_nxt.read());
            mac_en_sig.write(mac_en_nxt.read());
            red_en_sig.write(red_en_nxt.read());

            sum_reg.write(sum_nxt.read());
            // printf("SUM RESULT = %f\n", sum_reg.read());

            
            if(tbl_en_sig.read() && (nelems >= (N_LEARNERS * N_LANES))){
                // printf("YESSSSS\n");
                for(int learner=0; learner<N_LEARNERS; learner++){
                    for(int lane=0; lane<N_LANES; lane++){
                        // inputs_reg[learner][lane].write(inputs_nxt[learner][lane]);
                        inputs_reg[learner][lane].write(input_fifo.front());
                        input_fifo.pop_front();
                    }
                }
            }

        }

        wait();
    }
}



void I2CE_accelerator::comb_method()
{

    // printf("\n--- Comb method ---\n");

    tbl_en_nxt = en.read();
    mac_en_nxt = tbl_en_sig.read();
    red_en_nxt = mac_en_sig.read();

    // Packed indexes
    for(int lane=0; lane<N_LANES; lane++){
        packed_idx_nxt[lane] = packed_in[lane];
    }

    // Inputs
    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){
            codebook_nxt[learner][lane] = codebook_in[learner][lane];
            inputs_nxt[learner][lane] = inputs_in[learner][lane];
        }
    }


    float test_result = 0.0;


    for(int learn=0; learn<N_LEARNERS; learn++){
        for(int lane=0; lane<N_LANES; lane++){
            // printf("Input[%d] = %f\n", lane, inputs_reg[learn][lane].read());

            test_result += inputs_reg[learn][lane].read();
        }
    }

    sum_nxt.write(test_result);
}



I2CE_accelerator *
gem5::I2CE_acceleratorParams::create() const
{
    I2CE_accelerator *acc = new I2CE_accelerator(name.c_str());
    return acc;
}