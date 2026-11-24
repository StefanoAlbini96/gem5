#include "get_idx.hh"


void get_idx::clock_thread()
{

    // Initialize //


    // mask_reg = IDX_MASK;    // Remains constant
    mask_reg.write(0);
    bits_per_idx.write(2);   

    pred_last_active_lane_reg.write(0);


    for(int lane=0; lane<simd_lanes; lane++){
        idxs_reg[lane] = 0;
        // shamt_reg[lane] = (lane * bits_per_idx.read());
    }
    sel_reg = 0;

    shamt_reg.write(0);

    idx_en_reg.write(false);
    idx_en_prev_reg.write(false);

    en_cc_counter_reg.write(0);

    wait();


    // Clocked behaviour
    while(1)
    {

        mask_reg.write(bit_mask.read());
        bits_per_idx.write(idx_bits_in.read()); 

        idx_en_reg.write(idx_en_nxt.read());
        idx_en_prev_reg.write(idx_en_prev_nxt.read());
        
        en_cc_counter_reg.write(en_cc_counter_nxt.read());

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
    
    sc_uint<10> shamt_mod = last_shamt_possible + bits_per_idx.read();
    if(shamt_mod == 0){
        shamt_mod = 1;  // Guarantees we never divide by 0
    }

    // sc_uint<IDXS_PER_LANE_BITS> indexes_per_lane = (LANE_BITS / idx_bits_in);       // How many indexes in each lane
    

    // printf("\n====\n");
    // printf("idx_en_reg = %s\n", idx_en_reg.read() ? "true" : "false");
    // printf("idxs_per_lane = %d\n", idxs_per_lane.read().to_uint());
    // printf("bits_per_idx = %d\n", bits_per_idx.read().to_uint());
    // printf("last_shamt_possible = %d\n", last_shamt_possible.to_uint());
    // printf("shamt_mod = %d\n", shamt_mod.to_uint());
    // printf("sel_reg = %d\n", sel_reg.read().to_uint());
    // printf("shamt_reg = %d\n", shamt_reg.read().to_uint());
    

    // To check that we found the last active lane, in case subsequent one have the same condition
    bool last_act_lane_found = false;

    sel_updated = sel_reg.read();

    for(int lane=0; lane<simd_lanes; lane++){

        // printf("\n ----- lane  = %u -----\n", lane);

        // printf("shamt_reg = %u\n", shamt_reg.read().to_uint());

        // Get the correct lane of packed indexes        
        // packed_lane = packed_indexes[sel_reg.read()].read();
        packed_lane = packed_indexes[sel_updated].read();



        // If the shift register spills, then I behave like starting from the beginning
        uint16_t lane_shamt_tmp = shamt_reg.read() + (lane * bits_per_idx.read());
        

        // if(lane_shamt_tmp > last_shamt_possible){
        //     lane_shamt = lane_shamt_tmp - (last_shamt_possible + bits_per_idx.read());      
        // } else {
        //     lane_shamt = lane_shamt_tmp;
        // }
        
        lane_shamt = lane_shamt_tmp % shamt_mod;
        // printf("SHAMT = %d mod %d = %d\n", lane_shamt_tmp, shamt_mod.to_uint(), lane_shamt.to_uint());

        index = packed_lane >> lane_shamt;
        index = index & mask_reg.read();

        // printf("%d --> %d\n", packed_lane.to_uint(), index);
        // printf("%d\n", lane_shamt.to_uint());

        // uint8_t shifted = (packed_lane >> lane_shamt);


        idxs_nxt[lane] = index;

        // Updated shamt for the lane
        shamt_updated = shamt_reg.read() + (lane * bits_per_idx.read()) + (simd_lanes * bits_per_idx.read());

        // Write the output
        out[lane].write(idxs_reg[lane].read());


        if ((lane_shamt == last_shamt_possible) && !last_act_lane_found){
            last_active_lane_next = (lane);
            sel_updated = (sel_updated + 1) % simd_lanes;
            // printf("update ==> %d\n", sel_updated.to_uint());
        }

        // printf("last_act_lane_found = %d\n", last_act_lane_found);

        // printf("\tlane = %d shamnt = %u --> shamt updated = %u -> %u  |  sel_updated = %u\n", lane, lane_shamt.to_uint(), shamt_reg.read().to_uint(), shamt_updated.to_uint(), sel_updated.to_uint());
    }

    uint16_t lane_shamt_tmp = shamt_reg.read() + ((simd_lanes) * bits_per_idx.read());
    // if(lane_shamt_tmp > last_shamt_possible){
    //     lane_shamt = lane_shamt_tmp - (last_shamt_possible + bits_per_idx.read());      
    // } else {
    //     lane_shamt = lane_shamt_tmp;
    // }

    lane_shamt = lane_shamt_tmp % shamt_mod;
    shamt_nxt.write(lane_shamt);

    // printf("\n");
    // printf("sel_updated =  %d\n", sel_updated.to_uint());
    // printf("lane_shamt_tmp =  %d\n", lane_shamt_tmp);
    // printf("lane_shamt =  %d\n", lane_shamt.to_uint());
    // printf("====\n");
    sel_nxt.write(sel_updated);

    pred_last_active_lane_nxt.write(last_active_lane_next);

    bool en_next = idx_en_reg.read();

    // Rising edge of EN input
    bool en_rising = in_en.read() && !idx_en_prev_nxt.read();


    // If rising edge, start the internal EN
    if(!idx_en_reg.read() && en_rising){
        en_next = true;                 // Start the EN
        en_cc_counter_nxt.write(0);     // Reset the CC counter
    }


    if(idx_en_reg.read()){
        en_cc_counter_nxt.write(en_cc_counter_reg.read() + 1);  // Increment the counter
    }


    if(en_cc_counter_nxt.read() == (cc_active.read() - 1)){
        en_next = false;
    }


    // bool done_last_lane = (sel_updated < sel_reg.read());

    // // Stop when needed
    // if(idx_en_reg.read() && done_last_lane){
    //     en_next = false;
    // }

    // // Anticipating by 3 CC when there are no SIMD activations left
    // if(idx_en_reg.read() && (n_simd_left.read() == 3)){
    //     en_next = false;
    // }


    idx_en_nxt.write(en_next);
    
    idx_en_prev_nxt.write(in_en.read());


    last_active_lane.write(pred_last_active_lane_reg.read());

    en_out.write(idx_en_reg.read());

}

