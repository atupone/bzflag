#ifndef _MESSAGEQUEUS_H_
#define _MESSAGEQUEUS_H_

#include "config.h"

#include <stdint.h>
#ifdef HAVE_SDL2
#include "SDL_atomic.h"
#endif

#include "Protocol.h"

class MessageQueue
{
public:
    MessageQueue();
    ~MessageQueue();

    class Message
    {
    public:
        bool human;
        uint16_t code;
        uint16_t len;
        char msg[MaxPacketLen];
    };
    void reset();
    bool isFull();
    bool isEmpty();
    void saveHead(bool human, uint16_t code, uint16_t len);
    void dequeue();
    void getTail(
        bool &human,
        uint16_t &code,
        uint16_t &len,
        char *&msg);
    char *getHead();
private:
    static const unsigned int queueSize = 128;
    Message messages[queueSize];
#ifdef HAVE_SDL2
    SDL_atomic_t atomHead;
    SDL_atomic_t atomTail;
#endif
    int tail;
    int head;
    int next;
};
#endif
