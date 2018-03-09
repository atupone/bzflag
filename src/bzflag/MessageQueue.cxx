#include "MessageQueue.h"

#include "ErrorHandler.h"

MessageQueue::MessageQueue()
{
}

MessageQueue::~MessageQueue()
{
}

void MessageQueue::reset()
{
#ifdef HAVE_SDL2
    SDL_AtomicSet(&atomHead, 0);
    SDL_AtomicSet(&atomTail, 0);
#endif
    head = 0;
    tail = 0;
    next = 1;
}

bool MessageQueue::isFull()
{
#ifdef HAVE_SDL2
    int myTail = SDL_AtomicGet(&atomTail);
#else
    int myTail = tail;
#endif

    return myTail == next;
}

char *MessageQueue::getHead()
{
    return messages[head].msg;
}

void MessageQueue::saveHead(bool human, uint16_t code, uint16_t len)
{
    messages[head].human = human;
    messages[head].code  = code;
    messages[head].len   = len;
    head = next;
#ifdef HAVE_SDL2
    SDL_AtomicSet(&atomHead, head);
#endif
    next++;
    next %= queueSize;
}

void MessageQueue::dequeue()
{
    tail++;
    tail %= queueSize;
#ifdef HAVE_SDL2
    SDL_AtomicSet(&atomTail, tail);
#endif
}

void MessageQueue::getTail(
    bool &human,
    uint16_t &code,
    uint16_t &len,
    char *&msg)
{
    human = messages[tail].human;
    code  = messages[tail].code;
    len   = messages[tail].len;
    msg   = messages[tail].msg;
}

bool MessageQueue::isEmpty()
{
#ifdef HAVE_SDL2
    int myHead = SDL_AtomicGet(&atomHead);
#else
    int myHead = head;
#endif

    return tail == myHead;
}
