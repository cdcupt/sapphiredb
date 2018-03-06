#include <iostream>
#include <fstream>
#include "test.helloworld.pb.h"

void listMsg(const test::helloworld& msg) {
    std::cout << msg.id() << std::endl;
    std::cout << msg.name() << std::endl;
}

int main(void) {
    test::helloworld msg;

    std::fstream input("./log", std::ios::in | std::ios::binary);
    if(!msg.ParseFromIstream(&input)) {
        std::cerr << "Failed to parse address book." << std::endl;
        return -1;
    }

    listMsg(msg);

    return 0;
}
