#include "reduce.hh"



void reduce_mod::clock_thread()
{

    // Initialize
    for(int learner=0; learner<N_LEARNERS; learner++){
        reduced_reg[learner] = 0.0;
    }

    wait();

    // Clocked behaviour
    while(1){
        if(red_en){
            for(int learner=0; learner<N_LEARNERS; learner++){

                reduced_reg[learner].write(reduced_nxt[learner]);
            }
        }

        wait();
    }

}



void reduce_mod::comb_method()
{

    for(int learner=0; learner<N_LEARNERS; learner++){

        float res_tmp = 0.0;

        for(int lane=0; lane<N_LANES; lane++){
            res_tmp = res_tmp + mac_res[learner][lane].read();
        }
        reduced_nxt[learner].write(res_tmp);


        out[learner].write(reduced_reg[learner].read());
    }
}


