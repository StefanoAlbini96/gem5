


#include "cpu/minor/cusFU_sve_tblMAC.hh"

#include "arch/arm/system.hh"


using namespace gem5;



//////////////////////////////////
//        Codebook class        //


Codebook::Codebook(int size = 4){
    this->cb_size = size;
    this->values.resize(this->cb_size, 0);
}

void 
Codebook::load_codebook(float *cb_ptr){

    for(int i=0; i<this->cb_size; i++){
        this->values[i] = cb_ptr[i];
    }
}

void 
Codebook::load_value(int idx, float val){
    this->values[idx] = val;
}


vector<float> 
Codebook::get_values(){
return this->values;
}


vector<float> 
Codebook::tbl(vector<int> indexes){

    vector<float> res(indexes.size());

    for(int i=0; i<indexes.size(); i++){
        res[i] = this->values[indexes[i]];
    }
    return res;
}

void 
Codebook::print_cb(){
    printf("\nPrinting CB:\n");
    for(int i=0; i<this->cb_size; i++){
        printf("[%d]  %f\n", i, this->values[i]);
    }
}

//                              //
//////////////////////////////////


// ------------------------------------------------------ //


//////////////////////////////////
//       Custom FU              //


CusFU_SVE_tblMAC::CusFU_SVE_tblMAC(const std::string &name,
                                    const MinorFU &description,
                                    MinorCPU &cpu,
                                    int n_cb)
  : FUPipeline(name, description, cpu),
    codebooks(n_cb), 
    n_codebooks(n_cb),
    inputs(n_cb, vector<float>(n_cb, 0.0f)),
    accumulators(n_cb, vector<float>(n_cb, 0.0f)),
    out_vals(n_cb, 0.0f) {}




Codebook* 
CusFU_SVE_tblMAC::get_CB_by_index(int idx){

    if(idx >= this->n_codebooks){
        printf("Error (get_CB_by_index)! idx > n_codebooks in FU! (%d > %d)\n", idx, this->n_codebooks);
        exit(1);
    }

    return &(this->codebooks[idx]);
}


vector<float>* 
CusFU_SVE_tblMAC::get_input_reg_by_index(int idx){

    if(idx >= this->n_codebooks){
        printf("Error (get_input_reg_by_index)! idx > n_codebooks in FU! (%d > %d)\n", idx, this->n_codebooks);
        exit(1);
    }

    return &(this->inputs[idx]);
}


vector<float>* 
CusFU_SVE_tblMAC::get_acc_by_index(int idx){

    if(idx >= this->n_codebooks){
        printf("Error (get_input_reg_by_index)! idx > n_codebooks in FU! (%d > %d)\n", idx, this->n_codebooks);
        exit(1);
    }

    return &(this->accumulators[idx]);
}


void 
CusFU_SVE_tblMAC::tbl_MAC(vector<int> indexes){

    for(int i=0; i<this->n_codebooks; i++){

        vector<float> weights = this->codebooks[i].tbl(indexes);

        for(int j=0; j<weights.size(); j++){
            this->accumulators[i][j] += (this->inputs[i][j] * weights[j]);
        }
    }
}


void 
CusFU_SVE_tblMAC::tbl_MAC_lane(int index, int lane_num){

    for(int i=0; i<this->n_codebooks; i++){

        float weight = this->codebooks[i].get_values()[index];

        this->accumulators[i][lane_num] += (this->inputs[i][lane_num] * weight);
    }
}



void 
CusFU_SVE_tblMAC::reduce_acc(){

    for(int i=0; i<this->n_codebooks; i++){
        
        for(int j=0; j<this->accumulators[i].size(); j++){
            this->out_vals[i] += this->accumulators[i][j];
        }
    }
}



float 
CusFU_SVE_tblMAC::get_out_by_index(int idx){

    return this->out_vals[idx];
}


void 
CusFU_SVE_tblMAC::reset_acc_out(){

    for(int i=0; i<this->n_codebooks; i++){
        this->out_vals[i] = 0.0;

        for(int j=0; j<this->accumulators[i].size(); j++){
            this->accumulators[i][j] = 0.0;
        }
    }
}



void 
CusFU_SVE_tblMAC::print_in_by_idx(int idx){
    printf("\nPrinting In[%d]:\n", idx);
    
    for(int i=0; i<this->inputs[idx].size(); i++){
        printf("[%d] %f\n", i, this->inputs[idx].at(i));
    }
}

void 
CusFU_SVE_tblMAC::print_acc_by_idx(int idx){
    printf("\nPrinting Acc[%d]:\n", idx);

    for(int i=0; i<this->accumulators[idx].size(); i++){
        printf("[%d] %f\n", i, this->accumulators[idx].at(i));
    }
}

void 
CusFU_SVE_tblMAC::print_all_out(){
    printf("\nPrinting Out Vals:\n");

    for(int i=0; i<this->out_vals.size(); i++){
        printf("[%d] %f\n", i, this->out_vals.at(i));
    }
}


//                              //
//////////////////////////////////

