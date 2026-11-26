#include "tbl.hh"



void tbl::clock_thread()
{

    #ifdef GEM5
        int n_lanes = simd_lanes;
        int num_learners = n_learners;
    #else
        int n_lanes = N_LANES;
        int num_learners = N_LEARNERS;
    #endif

    // Initialize
    for(int learner=0; learner<num_learners; learner++){
        for(int lane=0; lane<n_lanes; lane++){
            weights_reg[learner][lane].write(0.0);
        }
    }

    wait();


    // Clocked behaviour
    while(1)
    {
        for(int learner=0; learner<num_learners; learner++){
            for(int lane=0; lane<n_lanes; lane++){

                if(tbl_en.read()){
                    weights_reg[learner][lane].write(weights_nxt[learner][lane].read());
                }
            }
        }

        wait();
    }
}


void tbl::comb_method()
{
    #ifdef GEM5
        int n_lanes = simd_lanes;
        int num_learners = n_learners;
    #else
        int n_lanes = N_LANES;
        int num_learners = N_LEARNERS;
    #endif

    sc_uint<IDX_BIT_MAX> index;

    for(int lane=0; lane<n_lanes; lane++){
        index = idxs[lane].read();
        
        for(int learner=0; learner<num_learners; learner++){


            weights_nxt[learner][lane].write(codebook[learner][index].read());
            
            out[learner][lane].write(weights_reg[learner][lane].read());
        }
    }
}


