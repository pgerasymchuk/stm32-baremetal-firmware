#include "stm32f103x6.h"
#include "timer.h"
#include "state_machine.h"

static volatile uint32_t system_time = 0;

typedef struct
{
    volatile uint8_t active;
    volatile uint16_t remaining_ms;
    uint32_t event_mask;
} SoftTimer;

static SoftTimer led_step_timer = {0U, 0U, SM_EVENT_LED_STEP};
static SoftTimer warning_timeout_timer = {0U, 0U, SM_EVENT_WARNING_TIMEOUT};
static SoftTimer window_timeout_timer = {0U, 0U, SM_EVENT_WINDOW_TIMEOUT};
static SoftTimer alarm_buzzer_step_timer = {0U, 0U, SM_EVENT_ALARM_BUZZER_STEP};
static SoftTimer action_long_timer = {0U, 0U, SM_EVENT_ACTION_LONG};

#define TIM2_PSC_1MS_TICK  (800U - 1U)   // 8 MHz / 800 = 10 kHz 
#define TIM2_ARR_1MS       (10U - 1U)    // 10 kHz / 10 = 1 kHz => 1 ms 

#define TIMX_PSC_1MS_TICK  (8000U - 1U)  // 8 MHz / 8000 = 1 kHz => 1 ms 

static void Timer_StartOneShotMs(TIM_TypeDef *tim, uint16_t duration_ms)
{
    if(duration_ms == 0U)
        duration_ms = 1U;

    tim->CR1 &= ~TIM_CR1_CEN;
    tim->CR1 |= TIM_CR1_URS;   // only overflow raises UIF, not the UG below
    tim->PSC = TIMX_PSC_1MS_TICK;
    tim->ARR = (uint32_t)duration_ms - 1U;
    tim->CNT = 0U;
    tim->EGR = TIM_EGR_UG;
    tim->SR &= ~TIM_SR_UIF;
    tim->DIER |= TIM_DIER_UIE;
    tim->CR1 |= TIM_CR1_OPM;
    tim->CR1 |= TIM_CR1_CEN;
}

static void Timer_Soft_Start(SoftTimer *timer, uint16_t duration_ms)
{
    if(duration_ms == 0U)
        duration_ms = 1U;

    timer->remaining_ms = duration_ms;
    timer->active = 1U;
}

static void Timer_Soft_Stop(SoftTimer *timer)
{
    timer->active = 0U;
    timer->remaining_ms = 0U;
}

static void Timer_Soft_Tick(SoftTimer *timer)
{
    if(!timer->active)
        return;

    if(timer->remaining_ms > 0U)
        timer->remaining_ms--;

    if(timer->remaining_ms == 0U)
    {
        timer->active = 0U;
        StateMachine_PostEventFromISR(timer->event_mask);
    }
}

void Timer_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM2->CR1 = 0U;
    TIM2->PSC = TIM2_PSC_1MS_TICK;
    TIM2->ARR = TIM2_ARR_1MS;
    TIM2->CNT = 0U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1 |= TIM_CR1_CEN;

    TIM3->CR1 |= TIM_CR1_OPM;
    TIM1->CR1 |= TIM_CR1_OPM;
    TIM3->SR &= ~TIM_SR_UIF;
    TIM1->SR &= ~TIM_SR_UIF;
    TIM3->DIER |= TIM_DIER_UIE;
    TIM1->DIER |= TIM_DIER_UIE;

    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(TIM1_UP_IRQn);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF;
        system_time++;

        Timer_Soft_Tick(&led_step_timer);
        Timer_Soft_Tick(&warning_timeout_timer);
        Timer_Soft_Tick(&window_timeout_timer);
        Timer_Soft_Tick(&alarm_buzzer_step_timer);
        Timer_Soft_Tick(&action_long_timer);
    }
}

void TIM3_IRQHandler(void)
{
    if(TIM3->SR & TIM_SR_UIF)
    {
        TIM3->SR &= ~TIM_SR_UIF;
        StateMachine_PostEventFromISR(SM_EVENT_RELAY_TIMEOUT);
    }
}

void TIM1_UP_IRQHandler(void)
{
    if(TIM1->SR & TIM_SR_UIF)
    {
        TIM1->SR &= ~TIM_SR_UIF;
        StateMachine_PostEventFromISR(SM_EVENT_BUZZER_SHOT_TIMEOUT);
    }
}

uint32_t millis(void)
{
    return system_time;
}

void Timer_RelayPulse_Start(uint16_t duration_ms)
{
    Timer_StartOneShotMs(TIM3, duration_ms);
}

void Timer_RelayPulse_Stop(void)
{
    TIM3->CR1 &= ~TIM_CR1_CEN;
    TIM3->SR &= ~TIM_SR_UIF;
}

void Timer_BuzzerShot_Start(uint16_t duration_ms)
{
    Timer_StartOneShotMs(TIM1, duration_ms);
}

void Timer_BuzzerShot_Stop(void)
{
    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM1->SR &= ~TIM_SR_UIF;
}

void Timer_LedStep_Start(uint16_t duration_ms)
{
    Timer_Soft_Start(&led_step_timer, duration_ms);
}

void Timer_LedStep_Stop(void)
{
    Timer_Soft_Stop(&led_step_timer);
}

void Timer_WarningTimeout_Start(uint16_t duration_ms)
{
    Timer_Soft_Start(&warning_timeout_timer, duration_ms);
}

void Timer_WarningTimeout_Stop(void)
{
    Timer_Soft_Stop(&warning_timeout_timer);
}

void Timer_WindowTimeout_Start(uint16_t duration_ms)
{
    Timer_Soft_Start(&window_timeout_timer, duration_ms);
}

void Timer_WindowTimeout_Stop(void)
{
    Timer_Soft_Stop(&window_timeout_timer);
}

void Timer_AlarmBuzzerStep_Start(uint16_t duration_ms)
{
    Timer_Soft_Start(&alarm_buzzer_step_timer, duration_ms);
}

void Timer_AlarmBuzzerStep_Stop(void)
{
    Timer_Soft_Stop(&alarm_buzzer_step_timer);
}

void Timer_ActionLong_Start(uint16_t duration_ms)
{
    Timer_Soft_Start(&action_long_timer, duration_ms);
}

void Timer_ActionLong_Stop(void)
{
    Timer_Soft_Stop(&action_long_timer);
}
