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



// template <typename elem_type>
// class Vector_register {

//   private:
//     uint8_t n_lanes;

//     vector<elem_type> values;


//   public:
//     Vector_register(uint8_t n_lanes);


//     vector<elem_type>* get_values();

//     uint8_t get_vect_len();
// };





// class Codebook {

//     private:
//         vector<float> values;
//         uint16_t cb_size;

//     public:
//         Codebook(int size);

//         void load_codebook(float *cb_ptr);
//         vector<float> get_values();
//         void load_value(int idx, float val);
//         vector<float> tbl(vector<uint32_t> indexes);

//         void print_cb();

// };







class Index_retrieval {

  private:

    uint8_t vect_len;

    uint8_t idx_per_lane;

    uint8_t bits_per_idx;

    uint8_t mask;

    /**
     * Vector of indexes packed in 32-bit words
     */
    vector<uint32_t> packed_indexes;

    /**
     * Word of packed indexes that is currently being processed
     */
    uint8_t current_indexes_lane;

    /**
     * Points to the position of the next index to process in a word of packed indexes (current_indexes_lane)
     */
    uint8_t idx_ptr;

    /**
     * Vector containing the current unpacked indexes to use to address the codebook
     */
    vector<uint32_t> cur_unpacked_idxs;



  public:
    Index_retrieval(uint8_t vect_len, uint8_t idx_per_lane, uint8_t bits_per_idx, uint8_t mask);


    /**
     * Used to load the packed indexes from the memory.
     * This function is called from the custom SVE microOp that rearranges
     * the interleaved data read from memory.
     *
     * @param lane_idx  :   index of the lane / element in the vector to be set
     */
    void set_packed_idxs_lane(uint8_t lane_idx, uint32_t new_value);

    vector<uint32_t> get_unpacked_idx();

    void advance_cur_idxs_lane();

    void mask_next_idxs(vector<bool> pred);

    void reset();

};





class Table_lookup {

  private:

    uint8_t vect_len;

    uint8_t n_codebooks;

    vector<vector<float>> codebooks;


    vector<vector<float>> weights;

  public:
    Table_lookup(uint8_t vect_len, uint8_t n_codebooks);

    void set_codebook_value(uint8_t cb_idx, uint8_t lane_idx, float value);

    vector<float> get_codebook_byIdx(uint8_t idx);

    vector<vector<float>> get_weights();


    void do_tbl(vector<uint32_t> indexes, vector<bool> pred);

};





class Compute {

  private:

    uint8_t vect_len;

    /**
     * Number of interleaved input vectors. Should be equal to the number of codebooks
     */
    uint8_t n_inputs;

    /**
     * Store the inputs (activations) for the different learners
     */
    vector<vector<float>> inputs;

    /**
     * Accumulate the results of the input-weights multiplication
     */
    vector<vector<float>> accumulators;


    /**
     * Final results of a chain of operations. The basically contain the reduced accumulators.
     * One element per each learner (interleaved)
     */
    vector<float> out_vals;


  public:
    Compute(uint8_t vect_len, uint8_t n_inputs);


    void set_inputs_lane(uint8_t in_idx, uint8_t lane_idx, float value);

    float get_out_val(uint8_t lane_idx);

    void vect_mult(vector<vector<float>> weights, vector<bool> pred);

    void reduce();

    void reset_out();

};








class CusFU_SVE_tblMAC : public FUPipeline
{
  private:

    uint8_t vect_len;

    Index_retrieval idx_ret;
    Table_lookup tbl_lu;
    Compute cmpt;


    /**
     * Gets set at the beginning of the process() method.
     * Determines which lanes are executed for the next processing iteration.
     */
    vector<bool> predicate;


  public:
    CusFU_SVE_tblMAC(const std::string &name,
                     const MinorFU &description,
                     MinorCPU &cpu,
                     uint8_t vect_len,
                     uint8_t idx_per_lane,
                     uint8_t n_cb,
                     uint8_t bits_per_idx,
                     uint8_t mask);


    void load_cb_lane(uint8_t cb_idx, uint8_t lane_idx, float value);


    void load_packed_idxs_lane(uint8_t lane_idx, uint32_t packed_idxs_lane);


    void load_inputs(uint8_t in_idx, uint8_t lane_idx, float value);


    float get_out_val(uint8_t lane_idx);


    void compute_step(uint8_t pred_upper_bound);


    void reduce_acc();


    void reset_out_vals();

};

}









// class CusFU_SVE_tblMAC : public FUPipeline
// {
//   private:

//     Index_retrieval *idx_ret;
//     Table_lookup *tbl_lu;
//     Compute *cmpt;




//     int n_codebooks;

//     vector<Codebook> codebooks;

//     uint32_t vec_len;

//     /**
//      * Vector of indexes packed in 32-bit words
//      */
//     vector<uint32_t> packed_indexes;

//     /**
//      * Word of packed indexes that is currently being processed
//      */
//     uint8_t current_indexes_lane;

//     /**
//      * Points to the position of the next index to process in a word of packed indexes (current_indexes_lane)
//      */
//     uint8_t idx_ptr;

//     /**
//      * Vector containing the current unpacked indexes to use to address the codebook
//      */
//     vector<uint32_t> cur_unpacked_idxs;


//     /**
//      * Gets set at the beginning of the process() method.
//      * Determines which lanes are executed for the next processing iteration.
//      */
//     vector<bool> predicate;

//     uint8_t bits_per_idx;
//     uint8_t mask;

//     vector<vector<float>> inputs;

//     vector<vector<float>> accumulators; // accumulate the results of the input-weights multiplications

//     vector<float> out_vals; // final results of a chain of operations. The basically contain the reduced accumulators



//   public:
//     CusFU_SVE_tblMAC(const std::string &name,
//                      const MinorFU &description,
//                      MinorCPU &cpu,
//                      int n_codebooks = 4,
//                      int vector_len = 4);


//     Codebook* get_CB_by_index(int idx);

//     vector<float>* get_input_reg_by_index(int idx);

//     vector<float>* get_acc_by_index(int idx);

//     vector<uint32_t>* get_packed_indexes();

//     void process(uint32_t missing_lanes, uint32_t missing_total);

//     void advance_cur_packed_idxs();

//     void mask_next_idxs();




//     void tbl_MAC();

//     void tbl_MAC_lane(int index, int lane_num);

//     /**
//      * Reduces the accumulator by summing the lanes.
//      */
//     void reduce_acc();

//     /**
//      * Returns one out value based on the passed index
//      */
//     float get_out_by_index(int idx);

//     /**
//      * Resets to 0 the entries of the accumulators and outputs
//      */
//     void reset_acc_out();

//     void print_in_by_idx(int idx);

//     void print_acc_by_idx(int idx);

//     void print_all_out();

//     void print_cur_unpkd_idxs();

// };

// }

#endif
