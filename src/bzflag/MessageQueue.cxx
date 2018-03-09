#include "MessageQueue.h"

#include "ErrorHandler.h"

void MessageQueue::lock()
{
#ifdef HAVE_SDL2
    SDL_LockMutex(mutex);
#endif
}

void MessageQueue::unlock()
{
#ifdef HAVE_SDL2
    SDL_UnlockMutex(mutex);
#endif
}

MessageQueue::MessageQueue()
{
#ifdef HAVE_SDL2
    mutex = SDL_CreateMutex();
    if (!mutex)
    {
        printFatalError("Error creating SDL_Mutex\n");
        exit(0);
    }
#endif
}

MessageQueue::~MessageQueue()
{
#ifdef HAVE_SDL2
    SDL_DestroyMutex(mutex);
#endif
}

void MessageQueue::reset()
{
    head = 0;
    tail = 0;
    next = 1;
}

bool MessageQueue::isFull()
{
    bool full;

    lock();
    full = tail == next;
    unlock();

    return full;
}

char *MessageQueue::getHead()
{
    return messages[head].msg;
}

bool MessageQueue::saveHead(bool human, uint16_t code, uint16_t len)
{
    bool full;

    messages[head].human = human;
    messages[head].code  = code;
    messages[head].len   = len;
    lock();
    head = next;
    next++;
    next %= queueSize;
    full = tail == next;
    unlock();
    return full;
}

bool MessageQueue::dequeue()
{
    bool empty;
    lock();
    tail++;
    tail %= queueSize;
    empty = tail == head;
    unlock();
    return empty;
}

void MessageQueue::getTail(
    bool &human,
    uint16_t &code,
    uint16_t &len,
    char *&msg)
{
    human = messages[tail].human;
    code = messages[tail].code;
    len = messages[tail].len;
    msg = messages[tail].msg;
}

bool MessageQueue::isEmpty()
{
    bool empty;
    lock();
    empty = tail == head;
    unlock();
    return empty;
}
