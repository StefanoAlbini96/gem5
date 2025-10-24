#ifndef __SRC_CPU_MINOR_MYADD_HH__
#define __SRC_CPU_MINOR_MYADD_HH__



// #pragma once



#include "cpu/minor/cpu.hh"
#include "cpu/minor/func_unit.hh"
#include "base/types.hh"
#include <iostream>

namespace gem5 {

using namespace minor;

class MinorCPU;


class MyFUPipeline : public FUPipeline
{
  private:
    int stored_value;


  public:
    MyFUPipeline(const std::string &name,
                 const MinorFU &description,
                 MinorCPU &cpu);



    int myadd(int x);


    int getStoredValue();
    void setStoredValue(int v);
};

}

#endif
