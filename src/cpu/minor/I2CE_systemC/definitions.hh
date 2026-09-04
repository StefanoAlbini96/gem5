#ifndef _DEFINITIONS_H_
#define _DEFINITIONS_H_


/**
 * Support LUT macro for log2 computation of power-of-2 numbers 
 */
#define log2_pow2(x) \
    x == 2  ? 1 : \
    x == 4  ? 2 : \
    x == 8  ? 3 : \
    x == 16 ? 4 : \
    x == 32 ? 5 : \
    x == 64 ? 6 : 0


/**
 * Support macro to generate the mask bits for the index
 */
#define mask_gen(x)         \
    x == 1  ? 0b1    :      \
    x == 2  ? 0b11   :      \
    x == 3  ? 0b111  :      \
    x == 4  ? 0b1111 : 0    \


/**
 * The number of learners in the model.
 * This reflects in the number of vector register in some modules.
 */
#define N_LEARNERS      2


/**
 * Bit-width of each lane element
 */
#define LANE_BITS        32

/**
 * The number of lanes per each vector/SIMD register
 */
#define N_LANES         4


/**
 * Number of entried per each codebooks.
 * Initially let's keep it set to N_LANES
 */
#define CB_SIZE         4


/**
 * Bit-width of each codebook index.
 */
#define IDX_BIT         (log2_pow2(CB_SIZE))


#define MY_MAC  5

/**
 * Mask for the codebook index extraction
 */
#define IDX_MASK        (mask_gen(IDX_BIT))


/**
 * Bit-width of the index to a specific lane
 */
#define LANE_IDX_BIT    (log2_pow2(N_LANES))


/**
 * How many indexes are packed in each lane
 */
#define IDX_PER_LANE    (LANE_BITS/IDX_BIT)
// #define IDX_PER_LANE    1


/**
 * Bit-width of the shift amount (for the get_idx module).
 * It depends on how many indexes I can pack in a lane.
 */
#define SHAMT_BIT       5


// Number of stages/modules of the accelerator, neede to propagate the EN signal
// I need 1 for the GET-IDX and TBL, and 2 for the MAC module (one for mul and one for add)
#define EN_STAGES    3


/**
 * Number of SIMD elements of the activation buffer (we have one buffer per learner)
*/
#define ACT_BUF_SIZE    16


// For the MAC module
#define MUL_STAGES      1
#define ADD_STAGES      1
#define MUL_EN          (MUL_STAGES)
#define ADD_EN          (ACT_BUF_SIZE)
// #define ADD_DRAIN_EN    (MUL_STAGES + ADD_STAGES + 2)
#define ADD_DRAIN_EN    (MUL_STAGES + ADD_STAGES + 1)


#endif