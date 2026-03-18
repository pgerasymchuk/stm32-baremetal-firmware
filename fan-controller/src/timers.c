#include "stm32f103x6.h"
#include "timers.h"
#include "events.h"

static volatile uint32_t systemTime = 0U;

void Timers_Init(void)
{
    /* 1 ms system tick (used for button debouncing). */
    SysTick_Config(SystemCoreClock / 1000U);

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* Timer tick: 10 kHz, status report period: 1 second. */
    TIM3->PSC = (SystemCoreClock / 10000U) - 1U;
    TIM3->ARR = 10000U - 1U;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->SR &= ~TIM_SR_UIF;

    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM3_IRQn);
}

void TIM3_IRQHandler(void)
{
    if (TIM3->SR & TIM_SR_UIF)
    {
        TIM3->SR &= ~TIM_SR_UIF;
        Events_Push(EVENT_STATUS_TIMER, 0U);
    }
}

void SysTick_Handler(void)
{
    systemTime++;
}

uint32_t millis(void)
{
    return systemTime;
}
