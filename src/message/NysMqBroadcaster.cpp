#include "NysMqBroadcaster.h"
#include "NysMqReceiver.h"

void NysMqBroadcaster::RegisterReceiver(NysMqReceiver *receiver)
{
    std::unique_lock lk(this->m_receivers);

    this->receivers.push_back(receiver);
}

void NysMqBroadcaster::UnregisterReceiver(NysMqReceiver *receiver)
{
    std::unique_lock lk(this->m_receivers);

    auto pos = std::find(this->receivers.begin(), this->receivers.end(), receiver);
    if(pos != this->receivers.end())
    {
        this->receivers.erase(pos);
    }
}

void NysMqBroadcaster::Broadcast(NysMessage message)
{
    std::unique_lock lk(this->m_receivers);

    for(auto receiver : this->receivers)
    {
        receiver->Push(message);
    }
}