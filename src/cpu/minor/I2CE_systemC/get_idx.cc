#include "get_idx.hh"


void get_idx::clock_thread()
{

    // Initialize
    for(int lane=0; lane<N_LANES; lane++){
        idxs_reg[lane] = 0;
        shamt_reg[lane] = (lane * IDX_BIT);
    }
    sel_reg = 0;

    mask_reg = IDX_MASK;    // Remains constant


    wait();


    // Clocked behaviour
    while(1)
    {
        // if(get_idx_en.read() && ready.read()){
        if(get_idx_en.read()){


            // For the duplicated packed lane
            for(int lane=0; lane<N_LANES; lane++){

                idxs_reg[lane].write(idxs_nxt[lane]);

                shamt_reg[lane].write(shamt_nxt[lane]);
            }

            // For the sel signal
            sel_reg = sel_nxt;
        }


        wait();
    }
}



void get_idx::comb_method()
{

    sc_dt::sc_uint<32>                        packed_lane;   // Lane of packed indexes to be unpacked
    uint8_t                         index;          // Unpacked index
    sc_uint<SHAMT_BIT>              shamt_prev;
    sc_uint<SHAMT_BIT>              shamt_updated;

    for(int lane=0; lane<N_LANES; lane++){

        // Get the correct lane of packed indexes        
        packed_lane = packed_indexes[sel_reg.read()].read();
        // printf("GET IDX [%d] <-- %d\n", packed_lane);


        // Shift and mask_reg
        index = packed_lane >> shamt_reg[lane].read();
        index = index & mask_reg.read();

        idxs_nxt[lane] = index;

        // shamt_prev = shamt_nxt[lane].read();
        shamt_prev = shamt_reg[lane].read();
        shamt_updated = shamt_reg[lane].read() + (N_LANES * IDX_BIT);
        shamt_nxt[lane].write(shamt_updated);

        // Overflow occurred --> need to use the next lane of packed indexes
        // printf("%d < %d\n", (uint32_t)shamt_updated, (uint32_t)shamt_prev);
        if (shamt_updated < shamt_prev){
            sel_nxt.write((sel_reg.read() + 1) % N_LANES);
        } else {
            sel_nxt.write(sel_reg.read());
        }

        // Write the output
        out[lane].write(idxs_reg[lane].read());
    }
}
