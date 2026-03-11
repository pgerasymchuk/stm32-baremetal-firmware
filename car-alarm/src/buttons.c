#include <stdint.h>

#include "stm32f103x6.h"
#include "buttons.h"
#include "gpio.h"
#include "timer.h"
#include "state_machine.h"

#define DEBOUNCE_TIME_MS   30U
#define LONG_PRESS_TIME_MS 800U

typedef struct
{
    uint8_t state;
    uint32_t last_change;
} DebouncedInput;

static volatile DebouncedInput buttons[BUTTON_COUNT];
static volatile DebouncedInput motion_input = {0U, 0U};

static void Button_HandleInterrupt(uint8_t id)
{
    uint32_t now = millis();
    uint8_t raw = Button_Read(id);

    if(raw != buttons[id].state)
    {
        if(now - buttons[id].last_change >= DEBOUNCE_TIME_MS)
        {
            buttons[id].state = raw;
            buttons[id].last_change = now;

            if(id == BUTTON_LOCK)
            {
                if(raw)
                    StateMachine_PostEventFromISR(SM_EVENT_LOCK_PRESSED);
            }
            else if(id == BUTTON_UNLOCK)
            {
                if(raw)
                    StateMachine_PostEventFromISR(SM_EVENT_UNLOCK_PRESSED);
            }
            else if(id == BUTTON_ACTION)
            {
                if(raw)
                    Timer_ActionLong_Start(LONG_PRESS_TIME_MS);
                else
                    Timer_ActionLong_Stop();
            }
        }
    }
}

static void Motion_HandleInterrupt(void)
{
    uint32_t now = millis();
    uint8_t raw = Motion_Read();

    if(raw != motion_input.state)
    {
        if(now - motion_input.last_change >= DEBOUNCE_TIME_MS)
        {
            motion_input.state = raw;
            motion_input.last_change = now;

            if(raw)
                StateMachine_PostEventFromISR(SM_EVENT_MOTION_ACTIVE);
            else
                StateMachine_PostEventFromISR(SM_EVENT_MOTION_IDLE);
        }
    }
}

void Buttons_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    AFIO->EXTICR[0] &= ~(AFIO_EXTICR1_EXTI0 | AFIO_EXTICR1_EXTI1 | AFIO_EXTICR1_EXTI2);
    AFIO->EXTICR[1] &= ~AFIO_EXTICR2_EXTI7;

    EXTI->IMR  |= EXTI_IMR_MR0  | EXTI_IMR_MR1  | EXTI_IMR_MR2;
    EXTI->IMR  |= EXTI_IMR_MR7;

    EXTI->RTSR |= EXTI_RTSR_TR0 | EXTI_RTSR_TR1 | EXTI_RTSR_TR2;
    EXTI->RTSR |= EXTI_RTSR_TR7;

    EXTI->FTSR |= EXTI_FTSR_TR0 | EXTI_FTSR_TR1 | EXTI_FTSR_TR2;
    EXTI->FTSR |= EXTI_FTSR_TR7;

    EXTI->PR = EXTI_PR_PR0 | EXTI_PR_PR1 | EXTI_PR_PR2 | EXTI_PR_PR7;

    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void EXTI0_IRQHandler(void)
{
    if(EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->PR = EXTI_PR_PR0;
        Button_HandleInterrupt(BUTTON_LOCK);
    }
}

void EXTI1_IRQHandler(void)
{
    if(EXTI->PR & EXTI_PR_PR1)
    {
        EXTI->PR = EXTI_PR_PR1;
        Button_HandleInterrupt(BUTTON_UNLOCK);
    }
}

void EXTI2_IRQHandler(void)
{
    if(EXTI->PR & EXTI_PR_PR2)
    {
        EXTI->PR = EXTI_PR_PR2;
        Button_HandleInterrupt(BUTTON_ACTION);
    }
}

void EXTI9_5_IRQHandler(void)
{
    if(EXTI->PR & EXTI_PR_PR7)
    {
        EXTI->PR = EXTI_PR_PR7;
        Motion_HandleInterrupt();
    }
}
