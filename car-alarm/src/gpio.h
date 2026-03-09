#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>

void GPIO_Init_All(void);

void LED_Set(uint8_t r, uint8_t g, uint8_t b);

void Relay_Lock_On(void);
void Relay_Lock_Off(void);

void Relay_Window_On(void);
void Relay_Window_Off(void);

void Buzzer_On(void);
void Buzzer_Off(void);

uint8_t Button_Read(uint8_t pin);
uint8_t Motion_Read(void);

#endif