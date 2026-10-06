#include "get_idx.hh"


void get_idx::clock_thread()
{

    // Initialize //


    // mask_reg = IDX_MASK;    // Remains constant
    mask_reg.write(0);
    bits_per_idx.write(0);   


    for(int lane=0; lane<simd_lanes; lane++){
        idxs_reg[lane] = 0;
        // shamt_reg[lane] = (lane * bits_per_idx.read());
    }
    sel_reg = 0;

    shamt_reg.write(0);

    idx_en_reg.write(false);
    idx_en_prev_reg.write(false);


    wait();


    // Clocked behaviour
    while(1)
    {

        mask_reg.write(bit_mask.read());
        bits_per_idx.write(idx_bits_in.read()); 

        idx_en_reg.write(idx_en_nxt.read());
        idx_en_prev_reg.write(idx_en_prev_nxt.read());
        
        if(idx_en_reg.read()){


            shamt_reg.write(shamt_nxt);

            // For the duplicated packed lane
            for(int lane=0; lane<simd_lanes; lane++){

                // shamt_reg[lane].write(shamt_nxt[lane]);
                idxs_reg[lane].write(idxs_nxt[lane]);

            }

            // For the sel signal
            sel_reg = sel_nxt;
        }


        wait();
    }
}



void get_idx::comb_method()
{

    sc_dt::sc_uint<32>              packed_lane;   // Lane of packed indexes to be unpacked
    uint8_t                         index;          // Unpacked index
    sc_uint<SHAMT_BIT>              shamt_prev;
    sc_uint<SHAMT_BIT>              shamt_updated;

    sc_uint<LANE_IDX_BIT_MAX>           sel_updated;

    sc_uint<SHAMT_BIT>              lane_shamt;

    for(int lane=0; lane<simd_lanes; lane++){

        // Get the correct lane of packed indexes        
        packed_lane = packed_indexes[sel_reg.read()].read();

        // Shift and mask_reg
        // index = packed_lane >> shamt_reg[lane].read();
        lane_shamt = shamt_reg.read() + (lane * bits_per_idx.read());
        index = packed_lane >> lane_shamt;
        index = index & mask_reg.read();

        idxs_nxt[lane] = index;

        // Updated shamt for the lane
        shamt_updated = shamt_reg.read() + (lane * bits_per_idx.read()) + (simd_lanes * bits_per_idx.read());

        // Overflow occurred --> need to use the next lane of packed indexes
        if (shamt_updated <= lane_shamt){
            sel_updated = (sel_reg.read() + 1) % simd_lanes;
        } else {
            sel_updated = sel_reg.read();
        }

        // Write the output
        out[lane].write(idxs_reg[lane].read());
    }
    
    // Updated shamt (base shamt)
    shamt_updated = shamt_reg.read() + (simd_lanes * bits_per_idx.read());
    shamt_nxt.write(shamt_updated);

    sel_nxt.write(sel_updated);


    bool en_next = idx_en_reg.read();

    // Rising edge of EN input
    bool en_rising = in_en.read() && !idx_en_prev_nxt.read();

    // If rising edge, start the internal EN
    if(!idx_en_reg.read() && en_rising){
        en_next = true;
    }


    bool done_last_lane = (sel_updated < sel_reg.read());

    // Stop when needed
    if(idx_en_reg.read() && done_last_lane){
        en_next = false;
    }


    idx_en_nxt.write(en_next);
    
    idx_en_prev_nxt.write(in_en.read());

}

