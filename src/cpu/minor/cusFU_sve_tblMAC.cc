


#include "cpu/minor/cusFU_sve_tblMAC.hh"

#include "arch/arm/system.hh"


using namespace gem5;



//////////////////////////////////
//        Codebook class        //


Codebook::Codebook(int size){
    this->cb_size = size;
    this->values.resize(this->cb_size, 0);
}

void 
Codebook::load_codebook(float *cb_ptr){

    for(int i=0; i<this->cb_size; i++){
        this->values[i] = cb_ptr[i];
    }
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
                                    int n_codebooks)
  : FUPipeline(name, description, cpu), stored_value(0),
    codebooks(n_codebooks), 
    n_codebooks(n_codebooks),
    accumulators(n_codebooks, vector<float>(4, 0.0f))
{
    this->stored_value = 0;
    vector<Codebook> codebooks;
    int n_codebooks;
    vector<vector<float>> accumulators; // accumulate the results of the input-weights multiplications

}


int
CusFU_SVE_tblMAC::myadd(int x){
    // printf("CUR-TICK = %d\n", curTick());
    int res = this->stored_value + x;
    // printf("CUR-TICK = %d\n", curTick());
    return res;
}


int
CusFU_SVE_tblMAC::getStoredValue() {
    return this->stored_value;
}

void
CusFU_SVE_tblMAC::setStoredValue(int v) {
    this->stored_value = v;
}





Codebook* 
CusFU_SVE_tblMAC::get_CB_by_index(int idx){
    if(idx >= this->n_codebooks){
        printf("Error! idx > n_codebooks in FU!\n");
        exit(1);
    }

    return &(this->codebooks[idx]);
}



void 
CusFU_SVE_tblMAC::tbl_MAC(vector<int> indexes, vector<float> input, int cb_index){

    // Get the weights by doing TBL to the codebook
    vector<float> weights = this->codebooks[cb_index].tbl(indexes);

    for(int i=0; i<weights.size(); i++){
        this->accumulators[cb_index][i] += (input[i] * weights[i]);
    }
}

float 
CusFU_SVE_tblMAC::reduce_and_return(int cb_index){
    float res = 0.0;

    for(int i=0; i<this->accumulators[cb_index].size(); i++){
        res += this->accumulators[cb_index][i];
    }
    return res;
}

//                              //
//////////////////////////////////

