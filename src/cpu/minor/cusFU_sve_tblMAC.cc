


#include "cpu/minor/cusFU_sve_tblMAC.hh"

#include "arch/arm/system.hh"


using namespace gem5;




//////////////////////////////////
//       Index_retrieval        //


Index_retrieval::Index_retrieval(uint8_t vect_len, uint8_t idx_per_lane, uint8_t bits_per_idx, uint8_t mask)
    :
    vect_len(vect_len),
    idx_per_lane(idx_per_lane),
    bits_per_idx(bits_per_idx),
    mask(mask),
    packed_indexes(vect_len, 0),
    current_indexes_lane(0),
    idx_ptr(0),
    cur_unpacked_idxs(vect_len, 0)
{}


void
Index_retrieval::set_packed_idxs_lane(uint8_t lane_idx, uint32_t new_value)
{
    this->packed_indexes[lane_idx] = new_value;
    // printf("New indexes packed: %d\n", this->packed_indexes[lane_idx]);

    // If we are loading new packed indexes, we should reset the internal state
    reset();
}


vector<uint32_t>
Index_retrieval::get_unpacked_idx(){
    return (this->cur_unpacked_idxs);
}


void
Index_retrieval::advance_cur_idxs_lane()
{
    // printf("Advancing cur idxs lane\n");
    if (this->current_indexes_lane < (this->vect_len - 1)){
        this->current_indexes_lane++;
        this->idx_ptr = 0;
        // printf("Cur indexes lane = %d\n", this->current_indexes_lane);
    } else {
        printf("ERROR! Cannot increase `current_indexes_lane` more!\n");
        exit(1);
    }
}

void
Index_retrieval::mask_next_idxs(vector<bool> pred)
{

    // Counts the number of active lanes
    uint8_t n_active = 0;
    for(int i=0; i<this->vect_len; i++){
        if(pred[i]){
            n_active++;
        }
    }

    // printf("Check if to advance ...\n");
    // Check if a new lane of packed indexes should be considered
    // printf("(%d + %d) >= %d ?\n", this->idx_ptr, n_active, this->idx_per_lane);
    if((this->idx_ptr + n_active) > this->idx_per_lane){
        this->advance_cur_idxs_lane();
    }

    uint32_t current_packed_idxs = this->packed_indexes[this->current_indexes_lane];
    // printf("Processing packed idxs: %d = [%d]\n", current_packed_idxs, this->current_indexes_lane);

    vector<uint32_t> masked_idxs(this->vect_len, 0);

    // printf("IDX PTR = %d\n", this->idx_ptr);

    for(int i=0; i<this->vect_len; i++){

        // predication
        if(pred[i]){
            masked_idxs[i] = (current_packed_idxs >> ((this->idx_ptr * this->bits_per_idx) + (i * this->bits_per_idx)));
            masked_idxs[i] &= this->mask;
        }
    }
    this->idx_ptr += n_active;

    // printf("INDEXES: \n");
    // for(int i=0; i<this->vect_len; i++){
    //     printf("--> %d\n", masked_idxs[i]);
    // }

    this->cur_unpacked_idxs = masked_idxs;
}


void
Index_retrieval::reset()
{
    this->idx_ptr = 0;
    this->current_indexes_lane = 0;
}


//                              //
//////////////////////////////////


// ------------------------------------------------------ //


//////////////////////////////////
//       Table_lookup           //


Table_lookup::Table_lookup(uint8_t vect_len, uint8_t n_codebooks)
    :
    vect_len(vect_len),
    n_codebooks(n_codebooks),
    codebooks(n_codebooks, vector<float>(vect_len, 0.0)),
    weights(n_codebooks, vector<float>(vect_len, 0.0))
{
    printf("N codebooks = %d\n", this->n_codebooks);
}


void
Table_lookup::set_codebook_value(uint8_t cb_idx, uint8_t lane_idx, float value)
{
    if(cb_idx >= this->n_codebooks){
        printf("ERROR! --> cb_idx > n_codebooks in Table_lookup::get_codebook_byIdx(). cb_idx = %d\n", cb_idx);
        exit(1);
    } else {
        this->codebooks[cb_idx][lane_idx] = value;
        // printf("Setting CB value [%d][%d] = %f\n", cb_idx, lane_idx, value);
    }
}



vector<float>
Table_lookup::get_codebook_byIdx(uint8_t idx)
{
    if(idx >= this->n_codebooks){
        printf("ERROR! --> idx > n_codebooks in Table_lookup::get_codebook_byIdx(). Idx = %d\n", idx);
        exit(1);
    } else {
        return this->codebooks[idx];
    }
}


vector<vector<float>>
Table_lookup::get_weights()
{
    return this->weights;
}


void
Table_lookup::do_tbl(vector<uint32_t> indexes, vector<bool> pred)
{
    for(int n_cb=0; n_cb<this->n_codebooks; n_cb++){

        for(int lane=0; lane<this->vect_len; lane++){

            if(pred[lane]){
                this->weights[n_cb][lane] = this->codebooks[n_cb][indexes[lane]];
                // printf("indexes[lane] = indexes[%d] = %d --> [%d]%f\n", lane, indexes[lane], n_cb, this->weights[n_cb][lane]);
            } else {
                this->weights[n_cb][lane] = 0.0;
            }
        }
    }
}


//                              //
//////////////////////////////////


// ------------------------------------------------------ //


//////////////////////////////////
//       Compute                //


Compute::Compute(uint8_t vect_len, uint8_t n_inputs)
    :
    vect_len(vect_len),
    n_inputs(n_inputs),
    inputs(n_inputs, vector<float>(vect_len, 0.0)),
    accumulators(n_inputs, vector<float>(vect_len, 0.0)),
    out_mem_tmp(n_inputs, 0.0),
    out_vals(n_inputs, 0.0)
    {}




void
Compute::set_inputs_lane(uint8_t in_idx, uint8_t lane_idx, float value)
{
    if(in_idx >= this->n_inputs){
        printf("ERROR! --> input idx > n_inputs in Compute::set_inputs_lane. in_idx = %d\n", in_idx);
        exit(1);
    } else {
        // printf("Setting IN[%d][%d] = %f\n", in_idx, lane_idx, value);
        this->inputs[in_idx][lane_idx] = value;
    }
}



void
Compute::set_out_mem_tmp(uint8_t in_idx, float value)
{
    // printf("Loading values TMP\n");
    if(in_idx >= this->n_inputs){
        printf("ERROR! --> in_idx > n_inputs in Compute::set_out_mem_tmp. lane_idx = %d\n", in_idx);
        exit(1);
    } else {
        this->out_mem_tmp[in_idx] = value;
        // printf("outTMP[%d] = %f\n", in_idx, this->out_mem_tmp[in_idx]);
    }

    // printf("Current values LD\n");
    // for(int i=0; i<this->n_inputs; i++){
    //     printf("[%d] %f\n", i, this->out_vals[i]);
    // }
}

void
Compute::add_outputs()
{
    // printf("Current values ADD\n");
    // for(int i=0; i<this->n_inputs; i++){
    //     printf("[%d] %f\n", i, this->out_vals[i]);
    // }

    // printf("Adding values...\n");
    for(int i=0; i<this->n_inputs; i++){
        // printf("[%d] %f + %f = ", i, this->out_vals[i], this->out_mem_tmp[i]);
        this->out_vals[i] += this->out_mem_tmp[i];
        // printf("%f\n", this->out_vals[i]);
    }

    // printf("DONE adding values!\n");
}



float
Compute::get_out_val(uint8_t lane_idx)
{

    // printf("Current values ST\n");
    // for(int i=0; i<this->n_inputs; i++){
    //     printf("[%d] %f\n", i, this->out_vals[i]);
    // }

    if(lane_idx >= this->n_inputs){
        printf("ERROR! --> lane_idx > n_inputs in Compute::get_out_val. lane_idx = %d\n", lane_idx);
        exit(1);
    } else {

        // printf("Getting value [%d] --> %f\n", lane_idx, this->out_vals[lane_idx]);
        return this->out_vals[lane_idx];
    }

}



void
Compute::vect_mult(vector<vector<float>> weights, vector<bool> pred)
{

    // printf("\nMultiplication:\n");
    for(int i=0; i<this->n_inputs; i++){
        // printf("--------------\n");
        for(int lane=0; lane<this->vect_len; lane++){

            if(pred[lane]){
                // printf("%f = %f + (%f * %f)\n", this->accumulators[i][lane] + (this->inputs[i][lane] * weights[i][lane]),
                //                                 this->accumulators[i][lane],
                //                                 this->inputs[i][lane],
                //                                 weights[i][lane]);
                this->accumulators[i][lane] += (this->inputs[i][lane] * weights[i][lane]);
            }
        }
    }

    // printf("\n");
    // for(int lane=0; lane<this->vect_len; lane++){
    //     printf("[%d] %f\n", lane, this->accumulators[0][lane]);
    // }

}



void
Compute::reduce()
{

    // printf("\n-- Reducing --\n");

    // for(int i=0; i<n_inputs; i++){
    //     printf("----\n");
    //     for(int lane=0; lane<this->vect_len; lane++){
    //         printf("%f\n", this->accumulators[i][lane]);
    //     }
    // }

    // printf("Accumulators:\n");
    for(int i=0; i<this->n_inputs; i++){
        for(int lane=0; lane<this->vect_len; lane++){
            this->out_vals[i] += this->accumulators[i][lane];
            // printf("[%d] %f\n", lane, this->accumulators[i][lane]);
        }
        // printf("%f\n", this->out_vals[i]);
        // printf("\n");

        // Reset the accumulators
        fill(this->accumulators[i].begin(), this->accumulators[i].end(), 0.0);
    }

    // printf("Final out:\n");
    // for(int i=0; i<this->n_inputs; i++){
    //     printf("Out %d --> %f\n", i, this->out_vals[i]);
    // }
}


void
Compute::reset_out()
{
    fill(this->out_vals.begin(), this->out_vals.end(), 0.0);
}

//                              //
//////////////////////////////////


// ------------------------------------------------------ //


//////////////////////////////////
//       Custom FU              //


CusFU_SVE_tblMAC::CusFU_SVE_tblMAC( const std::string &name,
                                    const MinorFU &description,
                                    MinorCPU &cpu,
                                    uint8_t vect_len,
                                    uint8_t idx_per_lane,
                                    uint8_t n_cb,
                                    uint8_t bits_per_idx,
                                    uint8_t mask)
    :
    FUPipeline(name, description, cpu),
    vect_len(vect_len),
    idx_ret(vect_len, idx_per_lane, bits_per_idx, mask),
    tbl_lu(vect_len, n_cb),
    cmpt(vect_len, n_cb),
    predicate(vect_len),
    unpkd_idxs(vect_len),
    w(n_cb, vector<float>(vect_len, 0.0))
    {}



void
CusFU_SVE_tblMAC::load_cb_lane(uint8_t cb_idx, uint8_t lane_idx, float value)
{
    this->tbl_lu.set_codebook_value(cb_idx, lane_idx, value);
}



void
CusFU_SVE_tblMAC::load_packed_idxs_lane(uint8_t lane_idx, uint32_t packed_idxs_lane)
{
    // printf("Setting packed idx = %d\n", packed_idxs_lane);
    this->idx_ret.set_packed_idxs_lane(lane_idx, packed_idxs_lane);
}


void
CusFU_SVE_tblMAC::load_inputs(uint8_t in_idx, uint8_t lane_idx, float value)
{
    this->cmpt.set_inputs_lane(in_idx, lane_idx, value);
}



void
CusFU_SVE_tblMAC::load_out_tmp(uint8_t in_idx, float value)
{
    this->cmpt.set_out_mem_tmp(in_idx, value);
}

void
CusFU_SVE_tblMAC::acc_out()
{
    this->cmpt.add_outputs();
}



float
CusFU_SVE_tblMAC::get_out_val(uint8_t lane_idx)
{
    return this->cmpt.get_out_val(lane_idx);
}


void
CusFU_SVE_tblMAC::compute_step(uint8_t pred_upper_bound)
{

    // printf("Computing step with upper_pred = %d\n", pred_upper_bound);

    // Compute the predicate
    vector<bool> pred(this->vect_len);

    for(int lane=0; lane<this->vect_len; lane++){
        if(lane<pred_upper_bound){
            pred[lane] = true;
        } else {
            pred[lane] = false;
        }
    }

    // Unpack the next group of indexes
    this->idx_ret.mask_next_idxs(pred);

    // Get the new unpacked indexes
    vector<uint32_t> unpacked_idxs = this->idx_ret.get_unpacked_idx();

    // Do the TBL on the codebooks (predicated)
    this->tbl_lu.do_tbl(unpacked_idxs, pred);

    // Get the weights
    vector<vector<float>> weights = this->tbl_lu.get_weights();

    // Do the weights-inputs multiplication
    this->cmpt.vect_mult(weights, pred);
}


void
CusFU_SVE_tblMAC::reduce_acc()
{
    this->cmpt.reduce();

    this->idx_ret.reset();
}

void
CusFU_SVE_tblMAC::reset_out_vals()
{
    this->cmpt.reset_out();
}




void
CusFU_SVE_tblMAC::getIdxs(uint8_t pred_upper_bound)
{
    // Compute the predicate
    for(int lane=0; lane<this->vect_len; lane++){
        if(lane<pred_upper_bound){
            this->predicate[lane] = true;
        } else {
            this->predicate[lane] = false;
        }
    }

    // Unpack the next group of indexes
    this->idx_ret.mask_next_idxs(this->predicate);

    // Get the new unpacked indexes
    this->unpkd_idxs = this->idx_ret.get_unpacked_idx();
}

void
CusFU_SVE_tblMAC::doTbl()
{
    // printf(">>>TBL\n");

    // Do the TBL on the codebooks (predicated)
    this->tbl_lu.do_tbl(this->unpkd_idxs, this->predicate);

    // Get the weights
    this->w = this->tbl_lu.get_weights();
}

void
CusFU_SVE_tblMAC::doMac()
{
    // printf(">>>MAC\n");
    // Do the weights-inputs multiplication
    this->cmpt.vect_mult(this->w, this->predicate);
}




//                              //
//////////////////////////////////
