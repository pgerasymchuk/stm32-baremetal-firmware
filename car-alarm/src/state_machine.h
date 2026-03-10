#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

enum
{
    SM_EVENT_LOCK_PRESSED         = (1UL << 0),
    SM_EVENT_UNLOCK_PRESSED       = (1UL << 1),
    SM_EVENT_MOTION_ACTIVE        = (1UL << 2),
    SM_EVENT_MOTION_IDLE          = (1UL << 3),
    SM_EVENT_ACTION_LONG          = (1UL << 4),
    SM_EVENT_RELAY_TIMEOUT        = (1UL << 5),
    SM_EVENT_BUZZER_SHOT_TIMEOUT  = (1UL << 6),
    SM_EVENT_LED_STEP             = (1UL << 7),
    SM_EVENT_WARNING_TIMEOUT      = (1UL << 8),
    SM_EVENT_WINDOW_TIMEOUT       = (1UL << 9),
    SM_EVENT_ALARM_BUZZER_STEP    = (1UL << 10)
};

void StateMachine_Init(void);
void StateMachine_ProcessEvents(void);
void StateMachine_PostEventFromISR(uint32_t events);

#endif
