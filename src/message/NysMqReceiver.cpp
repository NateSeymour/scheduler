#include "NysMqReceiver.h"
#include "NysMqBroadcaster.h"

NysMqReceiver::NysMqReceiver(NysMqBroadcaster *broadcaster)
{
    this->broadcaster = broadcaster;
    this->broadcaster->RegisterReceiver(this);
}

NysMqReceiver::~NysMqReceiver()
{
     this->broadcaster->UnregisterReceiver(this);
}

void NysMqReceiver::Push(NysMessage message)
{
    this->m_queue.lock();
    this->queue.push(message);
    this->m_queue.unlock();

    this->cv_queue.notify_all();
}

NysMessage NysMqReceiver::Peek()
{
    std::unique_lock lk(this->m_queue);

    return this->queue.front();
}

NysMessage NysMqReceiver::Consume()
{
    std::unique_lock lk(this->m_queue);

    NysMessage message{};

    if(!this->queue.empty())
    {
        message = this->queue.front();
        this->queue.pop();
    }
    else
    {
        message.type = MESSAGE_NONE;
    }

    return message;
}

NysMessage NysMqReceiver::ConsumeNext()
{
    while(this->queue.empty())
    {
        std::unique_lock lk(this->m_queue);
        this->cv_queue.wait(lk);
    }

    return this->Consume();
}