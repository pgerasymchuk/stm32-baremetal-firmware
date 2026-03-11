#include "stm32f103x6.h"
#include "gpio.h"
#include "timer.h"
#include "buttons.h"
#include "state_machine.h"

int main(void)
{
    GPIO_Init_All();
    Timer_Init();
    StateMachine_Init();
    Buttons_Init();

    while(1)
    {
        __WFI();
        StateMachine_ProcessEvents();
    }
}
