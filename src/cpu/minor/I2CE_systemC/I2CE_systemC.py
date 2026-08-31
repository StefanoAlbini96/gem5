


from m5.objects.SystemC import SystemC_ScModule
from m5.params import *
from m5.SimObject import SimObject
from m5.objects.ClockedObject import ClockedObject
from m5.objects.ClockDomain import SrcClockDomain


# # This class is a subclass of sc_module, and all the special magic which makes
# # that work is handled in the base classes.
class I2CE_accelerator(SystemC_ScModule):
    type = "I2CE_accelerator"
    cxx_class = "I2CE_accelerator"
    cxx_header = "cpu/minor/I2CE_systemC/accelerator.hh"



# This is a standard gem5 SimObject class with no special accomodation for the
# fact that one of its parameters is a systemc object.
class I2CE_driver(SimObject):
# class I2CE_driver(ClockedObject):
    type = "I2CE_driver"
    cxx_class = "gem5::I2CE_driver"
    cxx_header = "cpu/minor/I2CE_systemC/I2CE_driver.hh"

    accel = Param.I2CE_accelerator(NULL, "I2CE SC accelerator ")

    # clk_domain = Param.SrcClockDomain(SrcClockDomain(clock='1GHz'),
    #                                    "Clock domain for the SystemC bridge")

    # This parameter will be a pointer to an instance of the class above.
    # fadder = Param.SystemC_Fadder("My float-adder for testing.")
    # delay = Param.Latency("1ns", "Time to wait between each word.")
