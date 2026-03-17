#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>

void ADC_Init(void);

uint16_t ADC_GetPotRaw(void);
uint16_t ADC_GetTempRaw(void);

#endif