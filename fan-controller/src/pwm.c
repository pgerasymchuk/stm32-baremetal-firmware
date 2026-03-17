#include "stm32f103x6.h"
#include "pwm.h"

#define PWM_MAX_DUTY_PERCENT 100U

static uint8_t ClampPercent(uint8_t percent)
{
    if (percent > PWM_MAX_DUTY_PERCENT)
        return (uint8_t)PWM_MAX_DUTY_PERCENT;

    return percent;
}

void PWM_Init()
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // TIM2 default mapping: CH2 -> PA1. 
    AFIO->MAPR &= ~(3U << 8);

    // PA1 = TIM2_CH2 for LED PWM. 
    GPIOA->CRL &= ~(0xFU << 4);
    GPIOA->CRL |=  (0xBU << 4);

    // PA8 = TIM1_CH1 for fan PWM. 
    GPIOA->CRH &= ~(0xFU << 0);
    GPIOA->CRH |=  (0xBU << 0);

    // TIM2 channel 2 -> LED PWM. 
    TIM2->PSC = 71;
    TIM2->ARR = PWM_MAX_DUTY_PERCENT;

    TIM2->CCMR1 &= ~(TIM_CCMR1_CC2S | TIM_CCMR1_OC2M);
    TIM2->CCMR1 |= (6U << 12) | TIM_CCMR1_OC2PE;

    TIM2->CCER &= ~TIM_CCER_CC1E;
    TIM2->CCER |= TIM_CCER_CC2E;

    TIM2->CCR2 = 0;

    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->CR1 |= TIM_CR1_CEN;

    // TIM1 channel 1 -> fan PWM.
    TIM1->PSC = 71;
    TIM1->ARR = PWM_MAX_DUTY_PERCENT;

    TIM1->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);
    TIM1->CCMR1 |= (6U << 4) | TIM_CCMR1_OC1PE;

    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->CCR1 = 0;

    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->EGR = TIM_EGR_UG;
    TIM1->CR1 |= TIM_CR1_ARPE;
    TIM1->CR1 |= TIM_CR1_CEN;
}

void PWM_SetFan(uint8_t percent)
{
    TIM1->CCR1 = ClampPercent(percent);
}

void PWM_SetLED(uint8_t percent)
{
    TIM2->CCR2 = ClampPercent(percent);
}
