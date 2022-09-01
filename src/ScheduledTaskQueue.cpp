#include <algorithm>
#include "ScheduledTaskQueue.h"

void ScheduledTaskQueue::Add(ScheduledTask task)
{
    // Grab mutex
    this->m_queue.lock();

    // Add task safely and reorder the queue
    this->task_queue.push_back(std::move(task));

    std::sort(this->task_queue.begin(), this->task_queue.end(), [](const ScheduledTask& t1, const ScheduledTask& t2) -> bool {
        return t1.scheduled_time < t2.scheduled_time;
    });

    // Unlock mutex and notify change
    this->m_queue.unlock();
    this->cv_queue.notify_all();
}

size_t ScheduledTaskQueue::Count()
{
    return this->task_queue.size();
}

const ScheduledTask& ScheduledTaskQueue::Next()
{
    return this->task_queue.front();
}

ScheduledTask ScheduledTaskQueue::ConsumeNext()
{
    std::unique_lock lk(this->m_queue);

    ScheduledTask next = this->task_queue.front();

    this->task_queue.erase(this->task_queue.begin());

    return std::move(next);
}

void ScheduledTaskQueue::Poke()
{
    std::unique_lock lk(this->m_queue);
    this->cv_queue.notify_all();
}
