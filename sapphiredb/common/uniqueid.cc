#include "common/uniqueid.h"

uint64_t sapphiredb::common::Uniqueid::getTimestamps(){
    auto now = ::std::chrono::system_clock::now();
    return 0x7fffffffffc00000 & ((::std::chrono::duration_cast<::std::chrono::milliseconds>(now.time_since_epoch()).count()) << 22);
}

uint32_t sapphiredb::common::Uniqueid::rand(uint32_t min, uint32_t max, uint32_t seed){
    static ::std::default_random_engine e(seed);
    static ::std::uniform_real_distribution<double> u(min, max);
    return u(e);
}

uint32_t sapphiredb::common::Uniqueid::machineid = rand(0, 0x000004ff, time(NULL));

uint64_t sapphiredb::common::Uniqueid::getUniqueid(){
    int seqid = rand(0, 0x00000fff, time(NULL));
    return this->getTimestamps() | ((this->machineid) << 12) | seqid;
}
