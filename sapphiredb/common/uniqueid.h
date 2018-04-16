#ifndef SAPPHIREDB_COMMON_UNIQUEID_H_
#define SAPPHIREDB_COMMON_UNIQUEID_H_

#include <iostream>
#include <ctime>
#include <random>

#define LONG_CXX11

namespace sapphiredb
{
namespace common
{
class Uniqueid {
public:
    static Uniqueid& Instance() {
        static Uniqueid uid;
        return uid;
    }

    uint64_t getUniqueid();
public:
    Uniqueid();
    Uniqueid(Uniqueid const&);
    Uniqueid& operator=(Uniqueid const&);

    uint64_t getTimestamps();
    static uint32_t rand(uint32_t min, uint32_t max, uint32_t seed = 0);
    static uint32_t machineid;
};
} //namespace common
} //namespace sapphiredb

#endif
