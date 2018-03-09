#ifndef _MESSAGEQUEUS_H_
#define _MESSAGEQUEUS_H_

#include "config.h"

#include <stdint.h>
#ifdef HAVE_SDL2
#include "SDL_mutex.h"
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
    bool saveHead(bool human, uint16_t code, uint16_t len);
    bool dequeue();
    void getTail(
        bool &human,
        uint16_t &code,
        uint16_t &len,
        char *&msg);
    char *getHead();
private:
    void lock();
    void unlock();

    static const unsigned int queueSize = 128;
    Message messages[queueSize];
    unsigned int head;
    unsigned int tail;
    unsigned int next;
#ifdef HAVE_SDL2
    SDL_mutex *mutex;
#endif
};
#endif
