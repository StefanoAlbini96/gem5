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

    for(int learner=0; learner<4; learner++){
        out[learner].write(red_res[learner].read() + res_tmp_from_mem[learner].read());
    }
}


