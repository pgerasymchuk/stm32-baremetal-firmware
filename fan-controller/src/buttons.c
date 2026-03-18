#include "stm32f103x6.h"
#include "events.h"
#include "buttons.h"
#include "timers.h"

#define DEBOUNCE_TIME_MS 30U

static uint8_t buttonActiveLevel;
static volatile uint8_t buttonPressed = 0U;
static volatile uint32_t lastChange = 0U;

void Buttons_Init(void)
{
    /* Enable clocks */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    /* PB0 input pull-up */
    GPIOB->CRL &= ~(0xF << 0);
    GPIOB->CRL |=  (0x8 << 0);

    GPIOB->ODR |= (1 << 0);

    /* Map EXTI0 to PB0 */
    AFIO->EXTICR[0] &= ~(0xF << 0);
    AFIO->EXTICR[0] |=  (0x1 << 0);  // Port B

    /* Support both active-low and active-high wiring. */
    buttonActiveLevel = (GPIOB->IDR & (1U << 0)) ? 0U : 1U;

    /* Enable interrupt */
    EXTI->IMR  |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR |= EXTI_FTSR_TR0;

    NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->PR = EXTI_PR_PR0;

        uint32_t now = millis();
        uint8_t level = (GPIOB->IDR & (1U << 0)) ? 1U : 0U;
        uint8_t pressed = (level == buttonActiveLevel) ? 1U : 0U;

        /* Accept a level change only 30 ms after the previous one. */
        if ((pressed != buttonPressed) && (now - lastChange >= DEBOUNCE_TIME_MS))
        {
            buttonPressed = pressed;
            lastChange = now;

            if (pressed)
                Events_Push(EVENT_BUTTON_MODE, 0);
        }
    }
}
