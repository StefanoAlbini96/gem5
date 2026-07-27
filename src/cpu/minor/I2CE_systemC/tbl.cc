#include "tbl.hh"



void tbl::clock_thread()
{

    // Initialize
    for(int learner=0; learner<N_LEARNERS; learner++){
        for(int lane=0; lane<N_LANES; lane++){
            weights_reg[learner][lane].write(0.0);
        }
    }

    wait();


    // Clocked behaviour
    while(1)
    {
        for(int learner=0; learner<N_LEARNERS; learner++){
            for(int lane=0; lane<N_LANES; lane++){

                // if(tbl_en.read() && ready.read()){
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

    sc_uint<IDX_BIT> index;
    for(int lane=0; lane<N_LANES; lane++){
        index = idxs[lane].read();
        
        for(int learner=0; learner<N_LEARNERS; learner++){


            weights_nxt[learner][lane].write(codebook[learner][index].read());

            // printf("Index = %d --> cb[] = %f,  %f\n", (int)index, codebook_reg[index].read(), weights_nxt[lane].read());


            // printf("OUT = %f\n", out[lane].read());
            // out[lane] = weights_reg[lane].read();
            out[learner][lane].write(weights_reg[learner][lane].read());
        }
    }
}


