
#ifndef _I2CE_DRIVER_HH_
#define _I2CE_DRIVER_HH_

#include "sim/eventq.hh"

#include "sim/sim_object.hh"
#include "sim/clocked_object.hh"


// #include "cpu/func_unit.hh"
// #include "params/I2CE_Driver.hh"

#include "cpu/minor/I2CE_systemC/accelerator.hh"


#include "systemc/ext/channel/sc_buffer.hh"



namespace gem5 
{
class I2CE_driverParams;

class I2CE_driver : public gem5::SimObject
// class I2CE_driver : public gem5::ClockedObject
{

    public:
        I2CE_driver(const gem5::I2CE_driverParams &params);
        ~I2CE_driver() override;

        void drive();

        void stall();
        void enable();

        void push(float new_in);
        void compute_enable();
        void compute_disable();


        void ld_inputs(int learner, int lane, int in_idx, float input_val);

        void ld_codebooks(int learner, int lane, float cb_word);
        void load_packed_idxs(int lane, uint32_t idx_word);

        void push_tmp_res(int learner_id, float val);
        float get_out_value(int learner_id);


    private:
        I2CE_accelerator *accel;

        gem5::Tick delay;

        // not needed since the CPU called the accel //
        // gem5::MemberEventWrapper<&I2CE_driver::drive> event;

        sc_core::sc_clock           clk;
        // sc_core::sc_signal<bool>    clk;
        sc_core::sc_signal<bool>    rst;
        sc_core::sc_signal<bool>    en;

        // FIFO with the data coming from memory
        // sc_core::sc_fifo<float> input_fifo;
        // std::deque<float>           input_fifo;

        // This is the vector signal of the inputs that triggers the accelerator
        // sc_core::sc_signal<std::array<float, N_LANES>> input_vect;
        sc_core::sc_signal<float>   input_vect[N_LEARNERS][N_LANES][IDX_PER_LANE];



        sc_core::sc_signal<float>                   codebooks[N_LEARNERS][N_LANES];
        sc_core::sc_signal<sc_dt::sc_uint<32>>      packed_indexes[N_LANES];

        sc_core::sc_signal<float>                   red_out[N_LEARNERS];
        sc_core::sc_signal<float>                   res_tmp[N_LEARNERS];

        // sc_core::sc_signal<float> ab;

        void startup() override;


        // Tracefile
        sc_core::sc_trace_file *tf;

};


}




#endif