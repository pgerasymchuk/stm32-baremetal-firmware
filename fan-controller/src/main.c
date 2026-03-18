#include "stm32f103x6.h"

#include "state_machine.h"
#include "pwm.h"
#include "adc.h"
#include "uart.h"
#include "buttons.h"
#include "timers.h"

int main(void)
{
    SystemInit();

    UART_Init();

    PWM_Init();
    Buttons_Init();
    Timers_Init();
    ADC_Init();

    StateMachine_Init();

    while (1)
    {
        StateMachine_ProcessEvents();
        __WFI();
    }
}
