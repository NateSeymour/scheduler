#ifndef NOT_YOUR_SCHEDULER_NYSMQBROADCASTER_H
#define NOT_YOUR_SCHEDULER_NYSMQBROADCASTER_H

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <algorithm>
#include "NysMessage.h"

class NysMqReceiver;

class NysMqBroadcaster
{
private:
    std::mutex m_receivers;
    std::vector<NysMqReceiver*> receivers;

public:
    void RegisterReceiver(NysMqReceiver* receiver);
    void UnregisterReceiver(NysMqReceiver* receiver);

    void Broadcast(NysMessage message);
};

#endif //NOT_YOUR_SCHEDULER_NYSMQBROADCASTER_H
