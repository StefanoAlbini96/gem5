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
        void load_value(int idx, float val);
        vector<float> tbl(vector<int> indexes);

        void print_cb();

};













class CusFU_SVE_tblMAC : public FUPipeline
{
  private:

    int n_codebooks;

    vector<Codebook> codebooks;

    /**
     * Vector of indexes packed in 32-bit words
     */
    vector<uint32_t> packed_indexes;

    vector<vector<float>> inputs;

    vector<vector<float>> accumulators; // accumulate the results of the input-weights multiplications

    vector<float> out_vals; // final results of a chain of operations. The basically contain the reduced accumulators

  

  public:
    CusFU_SVE_tblMAC(const std::string &name,
                     const MinorFU &description,
                     MinorCPU &cpu,
                     int n_codebooks = 4);



    Codebook* get_CB_by_index(int idx);

    vector<float>* get_input_reg_by_index(int idx);

    vector<float>* get_acc_by_index(int idx);

    void tbl_MAC(vector<int> indexes);

    void tbl_MAC_lane(int index, int lane_num);

    /**
     * Reduces the accumulator by summing the lanes.
     */
    void reduce_acc();

    /**
     * Returns one out value based on the passed index
     */
    float get_out_by_index(int idx);

    /**
     * Resets to 0 the entries of the accumulators and outputs
     */
    void reset_acc_out();

    void print_in_by_idx(int idx);

    void print_acc_by_idx(int idx);

    void print_all_out();


};

}

#endif
