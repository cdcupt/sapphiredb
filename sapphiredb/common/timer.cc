#include "common/timer.h"

int64_t sapphiredb::common::Timer::CurrentTimeMillis()
{
    auto now = ::std::chrono::system_clock::now();
    return ::std::chrono::duration_cast<::std::chrono::milliseconds>(now.time_since_epoch()).count();
}

sapphiredb::common::Timer::Timer(){

}

sapphiredb::common::Timer::~Timer(){

}
