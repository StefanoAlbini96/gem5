#include "get_idx.hh"


void get_idx::clock_thread()
{

    // Initialize //


    // mask_reg = IDX_MASK;    // Remains constant
    mask_reg.write(0);
    bits_per_idx.write(0);   

    pred_last_active_lane_reg.write(0);


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

            pred_last_active_lane_reg.write(pred_last_active_lane_nxt.read());

            // For the sel signal
            sel_reg = sel_nxt;
        }


        wait();
    }
}



void get_idx::comb_method()
{

    // printf("=====\n");
    sc_dt::sc_uint<32>              packed_lane;   // Lane of packed indexes to be unpacked
    uint8_t                         index;          // Unpacked index
    
    
    sc_uint<SHAMT_BIT>              shamt_updated;
    // sc_uint<10>              shamt_updated;

    sc_uint<LANE_IDX_BIT_MAX>       sel_updated;

    sc_uint<SHAMT_BIT>              lane_shamt;

    sc_uint<LANE_IDX_BIT_MAX>       last_active_lane_next = simd_lanes_in.read();


    sc_uint<IDXS_PER_LANE_BITS> last_shamt_possible = (idxs_per_lane.read() * bits_per_idx.read()) - bits_per_idx.read();


    sel_updated = sel_reg.read();

    for(int lane=0; lane<simd_lanes; lane++){

        // printf("\n ----- lane  = %u -----\n", lane);

        // Get the correct lane of packed indexes        
        packed_lane = packed_indexes[sel_reg.read()].read();

        // Shift and mask_reg
        // index = packed_lane >> shamt_reg[lane].read();
        lane_shamt = shamt_reg.read() + (lane * bits_per_idx.read());
        index = packed_lane >> lane_shamt;
        index = index & mask_reg.read();


        uint8_t shifted = (packed_lane >> lane_shamt);
        // printf("Sel register = %u\n", sel_reg.read().to_uint());
        // printf("Packed lane = %u\n", packed_lane.to_uint());
        // printf("Lane shamt = %u\n", lane_shamt.to_uint());
        // printf("Packed lane shifted = %u\n", shifted);
        // std::cout << packed_lane.to_string(sc_dt::SC_BIN) << std::endl;
        // printf("Index = %u\n", index);

        idxs_nxt[lane] = index;

        // Updated shamt for the lane
        shamt_updated = shamt_reg.read() + (lane * bits_per_idx.read()) + (simd_lanes * bits_per_idx.read());

        // Overflow occurred --> need to use the next lane of packed indexes
        // if (shamt_updated <= lane_shamt){
        //     sel_updated = (sel_reg.read() + 1) % simd_lanes;
        // } else {
        //     sel_updated = sel_reg.read();
        // }

        if (lane_shamt == last_shamt_possible){
            last_active_lane_next = (lane);
            sel_updated = (sel_reg.read() + 1) % simd_lanes;
        }

        // printf("\tlane = %d shamnt = %u --> shamt updated = %u -> %u  |  sel_updated = %u\n", lane, lane_shamt.to_uint(), shamt_reg.read().to_uint(), shamt_updated.to_uint(), sel_updated.to_uint());

        // Write the output
        out[lane].write(idxs_reg[lane].read());
    }
    
    // printf("LAst shamt possilbe = %u * %u = %u   ||   %u\n", idxs_per_lane.read().to_uint(), bits_per_idx.read().to_uint(), last_shamt_possible.to_uint(), shamt_updated.to_uint());
    // if(shamt_updated >= last_shamt_possible){
    //     sel_updated = (sel_reg.read() + 1) % simd_lanes;
    // } else {
    //     sel_updated = sel_reg.read();
    // }


    // Updated shamt (base shamt)
    shamt_updated = shamt_reg.read() + (simd_lanes * bits_per_idx.read());
    if(shamt_updated < shamt_reg.read()){
        shamt_updated = 0;
    }
    shamt_nxt.write(shamt_updated);

    sel_nxt.write(sel_updated);

    pred_last_active_lane_nxt.write(last_active_lane_next);

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


    last_active_lane.write(pred_last_active_lane_reg.read());

}

