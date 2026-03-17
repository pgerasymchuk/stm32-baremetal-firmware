#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void PWM_Init(void);

void PWM_SetFan(uint8_t percent);
void PWM_SetLED(uint8_t percent);

#endif