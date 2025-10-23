


#include "cpu/minor/myadd.hh"

#include "arch/arm/system.hh"


using namespace gem5;


MyFUPipeline::MyFUPipeline(const std::string &name,
    const MinorFU &description,
    MinorCPU &cpu)
: FUPipeline(name, description, cpu), stored_value(0)
{
    this->stored_value = 100;
}


void 
MyFUPipeline::bindSysAndFU(ArmSystem *s){
    this->sys = s;
    // this->sys->setMyFUPipeline(this);
}


int
MyFUPipeline::myadd(int x){
    int res = this->stored_value + x;
    return res;
}


int 
MyFUPipeline::getStoredValue() { 
    return stored_value; 
}

void 
MyFUPipeline::setStoredValue(int v) {
    this->stored_value = v;
}