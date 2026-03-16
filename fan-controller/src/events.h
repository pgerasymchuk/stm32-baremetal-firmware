#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>

typedef enum
{
    EVENT_NONE = 0,
    EVENT_BUTTON_MODE,
    EVENT_POT_CHANGED,
    EVENT_TEMP_CHANGED,
    EVENT_UART_COMMAND,
    EVENT_STATUS_TIMER
} EventType;

typedef struct
{
    EventType type;
    uint16_t data;
} Event;

void Events_Push(EventType type, uint16_t data);
int  Events_Pop(Event *e);

#endif