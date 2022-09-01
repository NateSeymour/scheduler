#ifndef NOT_YOUR_SCHEDULER_SCHEDULEDTASK_H
#define NOT_YOUR_SCHEDULER_SCHEDULEDTASK_H

#include <chrono>
#include "Unit.h"

class ScheduledTask
{
public:
    std::chrono::time_point<std::chrono::system_clock> scheduled_time;
    std::shared_ptr<Unit> unit;

    ScheduledTask(std::chrono::time_point<std::chrono::system_clock> st, std::shared_ptr<Unit> u) : scheduled_time(st), unit(std::move(u)) {}
};

#endif //NOT_YOUR_SCHEDULER_SCHEDULEDTASK_H
