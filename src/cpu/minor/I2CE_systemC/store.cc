#include "store.hh"



void store_mod::clock_thread()
{

    wait();

    // Clocked behaviour
    while(1){
        wait();
    }

}



void store_mod::comb_method()
{

    for(int learner=0; learner<N_LEARNERS; learner++){
        out[learner].write(red_res[learner].read() + res_tmp_from_mem[learner].read());
    }
}


