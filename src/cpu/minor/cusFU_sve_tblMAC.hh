#ifndef __SRC_CPU_MINOR_MYADD_HH__
#define __SRC_CPU_MINOR_MYADD_HH__



// #pragma once



#include "cpu/minor/cpu.hh"
#include "cpu/minor/func_unit.hh"
#include "base/types.hh"
#include <iostream>
#include <numeric>
#include <vector>

using namespace std;

namespace gem5 {

using namespace minor;

class MinorCPU;






class Codebook {

    private:
        vector<float> values;
        uint16_t cb_size;

    public:
        Codebook(int size);

        void load_codebook(float *cb_ptr);
        vector<float> get_values();
        vector<float> tbl(vector<int> indexes);

        void print_cb();

};








class CusFU_SVE_tblMAC : public FUPipeline
{
  private:
    int stored_value;
    vector<Codebook> codebooks;
    int n_codebooks;
    vector<vector<float>> accumulators; // accumulate the results of the input-weights multiplications

  

  public:
    CusFU_SVE_tblMAC(const std::string &name,
                     const MinorFU &description,
                     MinorCPU &cpu,
                     int n_codebooks = 4);


                 
    int myadd(int x);


    int getStoredValue();
    void setStoredValue(int v);

    Codebook* get_CB_by_index(int idx);
    void tbl_MAC(vector<int> indexes, vector<float> input, int cb_index);
    float reduce_and_return(int cb_index);
};

}

#endif
