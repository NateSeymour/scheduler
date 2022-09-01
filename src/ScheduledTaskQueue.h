#ifndef NOT_YOUR_SCHEDULER_SCHEDULEDTASKQUEUE_H
#define NOT_YOUR_SCHEDULER_SCHEDULEDTASKQUEUE_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include "ScheduledTask.h"

class ScheduledTaskQueue
{
private:
    std::vector<ScheduledTask> task_queue;

public:
    std::mutex m_queue;
    std::condition_variable cv_queue;

    void Poke();
    const ScheduledTask& Next();
    ScheduledTask ConsumeNext();
    size_t Count();
    void Add(ScheduledTask task);
};

#endif //NOT_YOUR_SCHEDULER_SCHEDULEDTASKQUEUE_H
