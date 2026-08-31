#include "simd_mac.hh"



void simd_mac::clock_thread()
{

    // Initialize

    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){

            mul_res_reg[learner][lane] = 0.0;
            add_res_reg[learner][lane] = 0.0;
            res_reg[learner][lane] = 0.0;

            for(int m_stage=0; m_stage<MUL_STAGES; m_stage++){
                pipeline_mul[learner][lane][m_stage] = 0.0;
            }

            for(int add_stage=0; add_stage<ADD_STAGES; add_stage++){
                pipeline_add[learner][lane][add_stage] = 0.0;
                pipeline_add_drain[learner][lane][add_stage] = 0.0;
            }
        }
    }

    for(int mul_stage=0; mul_stage<MUL_EN; mul_stage++){
        mul_en_sig[mul_stage].write(false);
    }

    for(int add_stage=0; add_stage<ADD_DRAIN_EN; add_stage++){
        add_drain_en_sig[add_stage] = false;
    }



    wait();


    // Clocked behaviour
    while(1){


        // Update and shift the MUL enable
        for(int mul_stage=(MUL_EN-1); mul_stage>0; mul_stage--){
            mul_en_sig[mul_stage] = mul_en_sig[mul_stage-1];
        }
        mul_en_sig[0] = mac_en.read();

        // Update and shift the add drain enable
        for(int add_stage=(ADD_DRAIN_EN-1); add_stage>0; add_stage--){
            add_drain_en_sig[add_stage] = add_drain_en_sig[add_stage-1];
        }
        add_drain_en_sig[0] = add_en.read();



        /*
            mac_en --> directly check the input, we don't need any daley
            mul_en_sig --> delay the en to drain the MUL pipeline
        */
        // if(mac_en.read() || mul_en_sig[MUL_EN-1].read()){
        if(mac_en.read()){

            for(int learner=0; learner<N_LEARNERS; learner++){
                for(int lane=0; lane<N_LANES; lane++){

                    for(int m_stage=(MUL_STAGES-1); m_stage>0; m_stage--){
                        pipeline_mul[learner][lane][m_stage] = pipeline_mul[learner][lane][m_stage-1];
                    }
                    pipeline_mul[learner][lane][0] = mul_comb_res[learner][lane];

                    mul_res_reg[learner][lane].write(mul_res_nxt[learner][lane]);

                }
            }
        }


        /*
           add_drain_en_sig[0]  --> not directly the input so to give a 1 CC delay to the ADD enable
           add_drain_en_sig[-1] --> to properly drain the pipeline in the end
        */
        if(add_drain_en_sig[0].read() || add_drain_en_sig[ADD_DRAIN_EN-1].read()){

            for(int learner=0; learner<N_LEARNERS; learner++){
                for(int lane=0; lane<N_LANES; lane++){
                    for(int add_stage=(ADD_STAGES-1); add_stage>0; add_stage--){
                        pipeline_add[learner][lane][add_stage] = pipeline_add[learner][lane][add_stage-1];
                        pipeline_add_drain[learner][lane][add_stage] = pipeline_add_drain[learner][lane][add_stage-1];
                    }
                    pipeline_add[learner][lane][0] = add_comb_res[learner][lane];
                    pipeline_add_drain[learner][lane][0] = add_res_reg[learner][lane];

                    add_res_reg[learner][lane].write(add_res_nxt[learner][lane]);

                    res_reg[learner][lane].write(res_nxt[learner][lane]);
                }
            }
        }

        wait();
    }
}





void simd_mac::mult_comb_method()
{
    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){

            mul_comb_res[learner][lane] = activation[learner][lane].read() * weight[learner][lane].read();
            mul_res_nxt[learner][lane] = pipeline_mul[learner][lane][MUL_STAGES-1];

        }
    }
}




void simd_mac::add_comb_method()
{   

    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){

            float sum = 0.0;
            
            add_comb_res[learner][lane] = mul_res_reg[learner][lane].read() + add_res_reg[learner][lane].read();

            add_res_nxt[learner][lane] = pipeline_add[learner][lane][ADD_STAGES-1];

            for(int d_stage=0; d_stage<ADD_STAGES; d_stage++){
                sum += pipeline_add_drain[learner][lane][d_stage].read();
            }
            sum += add_res_reg[learner][lane].read();

            res_nxt[learner][lane] = sum;

            // Write the output
            out[learner][lane].write(res_reg[learner][lane].read());
        }
    }

    en_out.write(add_drain_en_sig[ADD_DRAIN_EN-1]);
}

