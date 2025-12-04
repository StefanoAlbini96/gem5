


#include "cpu/minor/cusFU_sve_tblMAC.hh"

#include "arch/arm/system.hh"


using namespace gem5;



// //////////////////////////////////
// //        Vector_register       //


// template <typename elem_type>
// Vector_register<elem_type>::Vector_register(uint8_t n_lanes)
//     : n_lanes(n_lanes),
//       values(n_lanes)
// {}



// template <typename elem_type>
// vector<elem_type>*
// Vector_register<elem_type>::get_values()
// {
//     return &(this->values();)
// }



// uint8_t get_vect_len();


// //                              //
// //////////////////////////////////


// ------------------------------------------------------ //


// //////////////////////////////////
// //        Codebook class        //


// Codebook::Codebook(int size = 4){
//     this->cb_size = size;
//     this->values.resize(this->cb_size, 0);
// }

// void
// Codebook::load_codebook(float *cb_ptr){

//     for(int i=0; i<this->cb_size; i++){
//         this->values[i] = cb_ptr[i];
//     }
// }

// void
// Codebook::load_value(int idx, float val){
//     this->values[idx] = val;
// }


// vector<float>
// Codebook::get_values(){
// return this->values;
// }


// vector<float>
// Codebook::tbl(vector<uint32_t> indexes){

//     vector<float> res(indexes.size());

//     for(int i=0; i<indexes.size(); i++){
//         res[i] = this->values[indexes[i]];
//     }
//     return res;
// }

// void
// Codebook::print_cb(){
//     printf("\nPrinting CB:\n");
//     for(int i=0; i<this->cb_size; i++){
//         printf("[%d]  %f\n", i, this->values[i]);
//     }
// }

// //                              //
// //////////////////////////////////


// ------------------------------------------------------ //



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
{}


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
    out_vals(n_inputs, 0.0)
    {}




void
Compute::set_inputs_lane(uint8_t in_idx, uint8_t lane_idx, float value)
{
    if(in_idx >= this->n_inputs){
        printf("ERROR! --> input idx > n_inputs in Compute::set_inputs_lane. in_idx = %d\n", in_idx);
        exit(1);
    } else {
        this->inputs[in_idx][lane_idx] = value;
    }
}



float
Compute::get_out_val(uint8_t lane_idx)
{
    if(lane_idx >= this->n_inputs){
        printf("ERROR! --> lane_idx > n_inputs in Compute::get_out_val. lane_idx = %d\n", lane_idx);
        exit(1);
    } else {
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
}



void
Compute::reduce()
{

    // printf("\nReduced:\n");
    for(int i=0; i<this->n_inputs; i++){
        for(int lane=0; lane<this->vect_len; lane++){
            this->out_vals[i] += this->accumulators[i][lane];
            // printf("%f + ", this->accumulators[i][lane]);
        }
        // printf("%f\n", this->out_vals[i]);

        // Reset the accumulators
        fill(this->accumulators[i].begin(), this->accumulators[i].end(), 0.0);
    }
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
    cmpt(vect_len, n_cb)
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

//                              //
//////////////////////////////////









// //////////////////////////////////
// //       Custom FU              //


// CusFU_SVE_tblMAC::CusFU_SVE_tblMAC(const std::string &name,
//                                     const MinorFU &description,
//                                     MinorCPU &cpu,
//                                     int n_cb,
//                                     int vector_len)
//   : FUPipeline(name, description, cpu),
//     n_codebooks(n_cb),
//     vec_len(vector_len),
//     codebooks(vector_len),  // each codebook has a size equal to the VL
//     packed_indexes(vector_len, 0),
//     current_indexes_lane(0),
//     predicate(vector_len, false),
//     idx_ptr(0),
//     cur_unpacked_idxs(vector_len, 0),
//     inputs(n_cb, vector<float>(vector_len, 0.0f)),
//     accumulators(n_cb, vector<float>(vector_len, 0.0f)),
//     out_vals(n_cb, 0.0f) {

//         bits_per_idx = (uint32_t) log(n_cb);
//         mask = (uint32_t) ((1 << bits_per_idx) - 1);
//     }




// Codebook*
// CusFU_SVE_tblMAC::get_CB_by_index(int idx){

//     if(idx >= this->n_codebooks){
//         printf("Error (get_CB_by_index)! idx > n_codebooks in FU! (%d > %d)\n", idx, this->n_codebooks);
//         exit(1);
//     }

//     return &(this->codebooks[idx]);
// }


// vector<float>*
// CusFU_SVE_tblMAC::get_input_reg_by_index(int idx){

//     if(idx >= this->n_codebooks){
//         printf("Error (get_input_reg_by_index)! idx > n_codebooks in FU! (%d > %d)\n", idx, this->n_codebooks);
//         exit(1);
//     }

//     return &(this->inputs[idx]);
// }


// vector<float>*
// CusFU_SVE_tblMAC::get_acc_by_index(int idx){

//     if(idx >= this->n_codebooks){
//         printf("Error (get_input_reg_by_index)! idx > n_codebooks in FU! (%d > %d)\n", idx, this->n_codebooks);
//         exit(1);
//     }

//     return &(this->accumulators[idx]);
// }



// vector<uint32_t>*
// CusFU_SVE_tblMAC::get_packed_indexes(){

//     return &(this->packed_indexes);
// }




// void
// CusFU_SVE_tblMAC::process(uint32_t missing_lane, uint32_t missing_total){

//     uint32_t upper_pred = 0;

//     if(missing_lane <= missing_total){
//         upper_pred = missing_lane;
//     } else {
//         upper_pred = missing_total;
//     }

//     // Update pred
//     uint8_t n_processed = 0;
//     for(int i=0; i<this->vec_len; i++){
//         if(i < upper_pred){
//             this->predicate[i] = true;
//             n_processed++;
//         }
//         else{
//             this->predicate[i] = false;
//         }
//     }

//     // Indexes retrieval
//     this->mask_next_idxs();

//     // TBL
//     this->tbl_MAC();

//     // Update the index pointer for the current lane
//     this->idx_ptr += n_processed;
// }


// void
// CusFU_SVE_tblMAC::advance_cur_packed_idxs(){

//     if(this->current_indexes_lane < this->vec_len){
//         this->current_indexes_lane++;
//         this->idx_ptr = 0;
//     } else {
//         printf("ERROR! Last lane reached, need to load more indexes!\n");
//         exit(1);
//     }

// }



// void
// CusFU_SVE_tblMAC::mask_next_idxs(){

//     uint32_t current_packed_idxs = this->packed_indexes[this->current_indexes_lane];
//     printf("Cur: %d\n", current_packed_idxs);

//     vector<uint32_t> masked_idxs(4, 0);

//     for(int i=0; i<this->vec_len; i++){

//         // predication
//         if(this->predicate[i]){
//             masked_idxs[i] = (current_packed_idxs >> ((this->idx_ptr * this->bits_per_idx) + (i * this->bits_per_idx)));
//             masked_idxs[i] &= this->mask;
//         }
//     }

//     printf("INDEXES: \n");
//     for(int i=0; i<this->vec_len; i++){
//         printf("--> %d\n", masked_idxs[i]);
//     }

//     this->cur_unpacked_idxs = masked_idxs;
// }






// void
// CusFU_SVE_tblMAC::tbl_MAC(){

//     for(int i=0; i<this->n_codebooks; i++){

//         vector<float> weights = this->codebooks[i].tbl(this->cur_unpacked_idxs);

//         for(int j=0; j<weights.size(); j++){
//             this->accumulators[i][j] += (this->inputs[i][j] * weights[j]);
//         }
//     }
// }


// void
// CusFU_SVE_tblMAC::tbl_MAC_lane(int index, int lane_num){

//     for(int i=0; i<this->n_codebooks; i++){

//         float weight = this->codebooks[i].get_values()[index];

//         this->accumulators[i][lane_num] += (this->inputs[i][lane_num] * weight);
//     }
// }



// void
// CusFU_SVE_tblMAC::reduce_acc(){

//     for(int i=0; i<this->n_codebooks; i++){

//         for(int j=0; j<this->accumulators[i].size(); j++){
//             this->out_vals[i] += this->accumulators[i][j];
//         }
//     }
// }



// float
// CusFU_SVE_tblMAC::get_out_by_index(int idx){

//     return this->out_vals[idx];
// }


// void
// CusFU_SVE_tblMAC::reset_acc_out(){

//     for(int i=0; i<this->n_codebooks; i++){
//         this->out_vals[i] = 0.0;

//         for(int j=0; j<this->accumulators[i].size(); j++){
//             this->accumulators[i][j] = 0.0;
//         }
//     }
// }



// void
// CusFU_SVE_tblMAC::print_in_by_idx(int idx){
//     printf("\nPrinting In[%d]:\n", idx);

//     for(int i=0; i<this->inputs[idx].size(); i++){
//         printf("[%d] %f\n", i, this->inputs[idx].at(i));
//     }
// }

// void
// CusFU_SVE_tblMAC::print_acc_by_idx(int idx){
//     printf("\nPrinting Acc[%d]:\n", idx);

//     for(int i=0; i<this->accumulators[idx].size(); i++){
//         printf("[%d] %f\n", i, this->accumulators[idx].at(i));
//     }
// }

// void
// CusFU_SVE_tblMAC::print_all_out(){
//     printf("\nPrinting Out Vals:\n");

//     for(int i=0; i<this->out_vals.size(); i++){
//         printf("[%d] %f\n", i, this->out_vals.at(i));
//     }
// }


// void
// CusFU_SVE_tblMAC::print_cur_unpkd_idxs(){
//     printf("\nPrinting current unpacked indexes:\n");

//     for(int i=0; i<this->vec_len; i++){
//         printf("[%d] %d\n", i, this->cur_unpacked_idxs.at(i));
//     }
// }



// //                              //
// //////////////////////////////////
