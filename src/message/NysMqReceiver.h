#ifndef NOT_YOUR_SCHEDULER_NYSMQRECEIVER_H
#define NOT_YOUR_SCHEDULER_NYSMQRECEIVER_H

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <algorithm>
#include "NysMessage.h"

class NysMqBroadcaster;

class NysMqReceiver
{
private:
    std::queue<NysMessage> queue;
    std::mutex m_queue;
    std::condition_variable cv_queue;

    NysMqBroadcaster* broadcaster;

public:
    void Push(NysMessage message);
    NysMessage Peek();
    NysMessage Consume();
    NysMessage ConsumeNext();

    template<class Rep, class Period>
    NysMessage ConsumeNextFor(const std::chrono::duration<Rep, Period>& rel_time)
    {
        while(this->queue.empty())
        {
            std::unique_lock lk(this->m_queue);
            this->cv_queue.wait_for(lk, rel_time);
        }

        return this->Consume();
    }

    template<class Clock, class Duration>
    NysMessage ConsumeNextUntil(const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        while(this->queue.empty())
        {
            std::unique_lock lk(this->m_queue);
            this->cv_queue.wait_until(lk, timeout_time);
        }

        return this->Consume();
    }

    NysMqReceiver() = delete;

    NysMqReceiver(NysMqBroadcaster* broadcaster);
    ~NysMqReceiver();
};

#endif //NOT_YOUR_SCHEDULER_NYSMQRECEIVER_H
