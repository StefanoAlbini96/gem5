#ifndef __SRC_CPU_MINOR_MYADD_HH__
#define __SRC_CPU_MINOR_MYADD_HH__



// #pragma once



#include "cpu/minor/func_unit.hh"
#include "cpu/minor/cpu.hh"
#include "base/types.hh"
#include <iostream>


using namespace gem5;
using namespace minor;

namespace gem5 {

class ArmSystem;


class MyFUPipeline : public FUPipeline
{
  private:
    int stored_value;
  

  ArmSystem *sys;

  public:
    MyFUPipeline(const std::string &name,
                 const MinorFU &description,
                 MinorCPU &cpu);


    void bindSysAndFU(ArmSystem *s);
                 
    int myadd(int x);


    int getStoredValue();
    void setStoredValue(int v);
};

}

#endif