#ifndef SAPPHIRE_COMMON_TIMER_H_
#define SAPPHIRE_COMMON_TIMER_H_

#include <chrono>
#include <thread>
#include <future>

namespace sapphiredb
{
namespace common
{

class Timer{
private:
    template<class F, class... Args>
    struct TimerNode{
        std::future<typename std::result_of<F(Args...)>::type> _callback;
    };
    int64_t CurrentTimeMillis();
public:
    Timer();
    ~Timer();
    
};
}// namespace common
}// namespace sapphire



#endif
