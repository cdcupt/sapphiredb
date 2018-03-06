#include <iostream>
#include <fstream>
#include "test.helloworld.pb.h"

int main(void){
    test::helloworld msg;
    msg.set_id(1);
    msg.set_name("cdcupt");

    std::fstream output("./log", std::ios::out | std::ios::trunc | std::ios::binary);

    if(!msg.SerializeToOstream(&output)){
        std::cerr << "Failed to write msg." << std::endl;
        return -1;
    }

    return 0;
}
