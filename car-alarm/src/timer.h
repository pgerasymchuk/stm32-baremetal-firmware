#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void Timer_Init(void);

uint32_t millis(void);

void Timer_RelayPulse_Start(uint16_t duration_ms);
void Timer_RelayPulse_Stop(void);

void Timer_BuzzerShot_Start(uint16_t duration_ms);
void Timer_BuzzerShot_Stop(void);

void Timer_LedStep_Start(uint16_t duration_ms);
void Timer_LedStep_Stop(void);

void Timer_WarningTimeout_Start(uint16_t duration_ms);
void Timer_WarningTimeout_Stop(void);

void Timer_WindowTimeout_Start(uint16_t duration_ms);
void Timer_WindowTimeout_Stop(void);

void Timer_AlarmBuzzerStep_Start(uint16_t duration_ms);
void Timer_AlarmBuzzerStep_Stop(void);

void Timer_ActionLong_Start(uint16_t duration_ms);
void Timer_ActionLong_Stop(void);

#endif
