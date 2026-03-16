#include "events.h"

#define EVENT_QUEUE_SIZE 32

static volatile Event queue[EVENT_QUEUE_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;

void Events_Push(EventType type, uint16_t data)
{
    uint8_t next = (head + 1) % EVENT_QUEUE_SIZE;

    if (next == tail)
        return;

    queue[head].type = type;
    queue[head].data = data;

    head = next;
}

int Events_Pop(Event *e)
{
    if (head == tail)
        return 0;

    *e = queue[tail];

    tail = (tail + 1) % EVENT_QUEUE_SIZE;

    return 1;
}