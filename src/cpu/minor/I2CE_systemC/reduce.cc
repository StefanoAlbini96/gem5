#include "reduce.hh"



void reduce_mod::clock_thread()
{

    #ifdef GEM5
        int num_learners = n_learners;
    #else
        int num_learners = N_LEARNERS;
    #endif

    // Initialize
    for(int learner=0; learner<num_learners; learner++){
        reduced_reg[learner] = (float_type)0.0;
    }

    wait();

    // Clocked behaviour
    while(1){
        if(red_en){
            for(int learner=0; learner<num_learners; learner++){

                reduced_reg[learner].write(reduced_nxt[learner]);
            }
        }

        wait();
    }

}



void reduce_mod::comb_method()
{


    #ifdef GEM5
        int n_lanes = simd_lanes;
        int num_learners = n_learners;
    #else
        int n_lanes = N_LANES;
        int num_learners = N_LEARNERS;
    #endif


    for(int learner=0; learner<num_learners; learner++){

        float_type res_tmp = (float_type)0.0;

        for(int lane=0; lane<n_lanes; lane++){
            res_tmp = res_tmp + mac_res[learner][lane].read();
        }

        // Add the previous result from memory
        res_tmp += res_from_mem[learner].read();

        reduced_nxt[learner].write(res_tmp);

        out[learner].write(reduced_reg[learner].read());
    }
}


