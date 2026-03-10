#include "stm32f103x6.h"
#include "state_machine.h"
#include "gpio.h"
#include "timer.h"

typedef enum
{
    STATE_OPEN,
    STATE_CLOSED,
    STATE_WARNING,
    STATE_ALARM,
    STATE_WINDOW_ACTION
} SystemState;

#define RELAY_PULSE_MS         250U
#define BUZZER_OPEN_MS         150U
#define BUZZER_CLOSE_MS        400U
#define BUZZER_WINDOW_MS       100U
#define WARNING_DURATION_MS    4000U
#define WARNING_LED_HALF_MS    200U
#define ALARM_LED_HALF_MS      100U
#define ALARM_BUZZER_HALF_MS   200U
#define MOTION_COUNT_FOR_ALARM 2U
#define WINDOW_RELAY_MS        6000U
#define CLOSED_LED_PERIOD_MS   1000U
#define CLOSED_LED_ON_MS       200U
#define WINDOW_LED_HALF_MS     150U

static SystemState state = STATE_OPEN;
static volatile uint32_t pending_events = 0U;

static uint32_t motion_count = 0U;
static uint8_t motion_latched = 0U;
static uint8_t led_phase_on = 0U;
static uint8_t alarm_buzzer_on = 0U;

void StateMachine_PostEventFromISR(uint32_t events)
{
    pending_events |= events;
}

static uint32_t StateMachine_FetchEvents(void)
{
    uint32_t events;

    __disable_irq();
    events = pending_events;
    pending_events = 0U;
    __enable_irq();

    return events;
}

static void Buzzer_Timed(uint16_t duration_ms)
{
    Buzzer_On();
    Timer_BuzzerShot_Start(duration_ms);
}

static void Relay_Lock_Pulse(void)
{
    Relay_Lock_On();
    Timer_RelayPulse_Start(RELAY_PULSE_MS);
}

static void EnterOpenState(void)
{
    state = STATE_OPEN;
    motion_count = 0U;
    motion_latched = 0U;
    alarm_buzzer_on = 0U;

    Timer_WarningTimeout_Stop();
    Timer_WindowTimeout_Stop();
    Timer_AlarmBuzzerStep_Stop();
    Timer_ActionLong_Stop();
    Timer_LedStep_Stop();

    LED_Set(0U, 1U, 0U);
}

static void EnterClosedState(void)
{
    state = STATE_CLOSED;
    motion_count = 0U;
    motion_latched = 0U;
    alarm_buzzer_on = 0U;

    Timer_WarningTimeout_Stop();
    Timer_WindowTimeout_Stop();
    Timer_AlarmBuzzerStep_Stop();
    Timer_ActionLong_Stop();

    led_phase_on = 1U;
    LED_Set(1U, 0U, 0U);
    Timer_LedStep_Start(CLOSED_LED_ON_MS);
}

static void EnterWarningState(void)
{
    state = STATE_WARNING;
    alarm_buzzer_on = 0U;

    Timer_WarningTimeout_Start(WARNING_DURATION_MS);
    Timer_WindowTimeout_Stop();
    Timer_AlarmBuzzerStep_Stop();
    Timer_ActionLong_Stop();

    led_phase_on = 1U;
    LED_Set(1U, 1U, 0U);
    Timer_LedStep_Start(WARNING_LED_HALF_MS);
}

static void EnterAlarmState(void)
{
    state = STATE_ALARM;
    alarm_buzzer_on = 1U;
    motion_latched = 1U;

    Timer_WarningTimeout_Stop();
    Timer_WindowTimeout_Stop();
    Timer_BuzzerShot_Stop();
    Timer_ActionLong_Stop();

    led_phase_on = 1U;
    LED_Set(1U, 0U, 0U);
    Timer_LedStep_Start(ALARM_LED_HALF_MS);

    Buzzer_On();
    Timer_AlarmBuzzerStep_Start(ALARM_BUZZER_HALF_MS);
}

static void EnterWindowActionState(void)
{
    state = STATE_WINDOW_ACTION;
    motion_latched = 0U;
    alarm_buzzer_on = 0U;

    Timer_WarningTimeout_Stop();
    Timer_AlarmBuzzerStep_Stop();
    Timer_ActionLong_Stop();

    Relay_Window_On();
    Buzzer_Timed(BUZZER_WINDOW_MS);
    Timer_WindowTimeout_Start(WINDOW_RELAY_MS);

    led_phase_on = 1U;
    LED_Set(0U, 0U, 1U);
    Timer_LedStep_Start(WINDOW_LED_HALF_MS);
}

static void HandleLedStepEvent(void)
{
    switch(state)
    {
        case STATE_OPEN:
            Timer_LedStep_Stop();
            LED_Set(0U, 1U, 0U);
            break;

        case STATE_CLOSED:
            if(led_phase_on)
            {
                led_phase_on = 0U;
                LED_Set(0U, 0U, 0U);
                Timer_LedStep_Start((uint16_t)(CLOSED_LED_PERIOD_MS - CLOSED_LED_ON_MS));
            }
            else
            {
                led_phase_on = 1U;
                LED_Set(1U, 0U, 0U);
                Timer_LedStep_Start(CLOSED_LED_ON_MS);
            }
            break;

        case STATE_WARNING:
            led_phase_on ^= 1U;
            LED_Set(led_phase_on, led_phase_on, 0U);
            Timer_LedStep_Start(WARNING_LED_HALF_MS);
            break;

        case STATE_ALARM:
            led_phase_on ^= 1U;
            LED_Set(led_phase_on, 0U, 0U);
            Timer_LedStep_Start(ALARM_LED_HALF_MS);
            break;

        case STATE_WINDOW_ACTION:
            led_phase_on ^= 1U;
            LED_Set(0U, 0U, led_phase_on);
            Timer_LedStep_Start(WINDOW_LED_HALF_MS);
            break;
    }
}

static void HandleEvents(uint32_t events)
{
    if(events & SM_EVENT_RELAY_TIMEOUT)
        Relay_Lock_Off();

    if(events & SM_EVENT_BUZZER_SHOT_TIMEOUT)
    {
        if(state != STATE_ALARM)
            Buzzer_Off();
    }

    if(events & SM_EVENT_LED_STEP)
        HandleLedStepEvent();

    if(events & SM_EVENT_ALARM_BUZZER_STEP)
    {
        if(state == STATE_ALARM)
        {
            alarm_buzzer_on ^= 1U;
            if(alarm_buzzer_on)
                Buzzer_On();
            else
                Buzzer_Off();
            Timer_AlarmBuzzerStep_Start(ALARM_BUZZER_HALF_MS);
        }
    }

    if(events & SM_EVENT_UNLOCK_PRESSED)
    {
        if((state == STATE_CLOSED) || (state == STATE_WARNING) || (state == STATE_ALARM))
        {
            Relay_Lock_Pulse();
            Timer_AlarmBuzzerStep_Stop();
            Buzzer_Timed(BUZZER_OPEN_MS);
            EnterOpenState();
            return;
        }
    }

    switch(state)
    {
        case STATE_OPEN:
            if(events & SM_EVENT_LOCK_PRESSED)
            {
                Relay_Lock_Pulse();
                Buzzer_Timed(BUZZER_CLOSE_MS);
                EnterClosedState();
                return;
            }

            if(events & SM_EVENT_ACTION_LONG)
            {
                EnterWindowActionState();
                return;
            }
            break;

        case STATE_CLOSED:
            if(events & SM_EVENT_MOTION_IDLE)
                motion_latched = 0U;

            if((events & SM_EVENT_MOTION_ACTIVE) && !motion_latched)
            {
                motion_latched = 1U;
                motion_count = 1U;
                EnterWarningState();
                return;
            }
            break;

        case STATE_WARNING:
            if(events & SM_EVENT_MOTION_IDLE)
                motion_latched = 0U;

            if((events & SM_EVENT_MOTION_ACTIVE) && !motion_latched)
            {
                motion_latched = 1U;
                motion_count++;
                if(motion_count >= MOTION_COUNT_FOR_ALARM)
                {
                    EnterAlarmState();
                    return;
                }
            }

            if(events & SM_EVENT_WARNING_TIMEOUT)
            {
                if(motion_count >= MOTION_COUNT_FOR_ALARM)
                {
                    EnterAlarmState();
                }
                else
                {
                    EnterClosedState();
                }
                return;
            }
            break;

        case STATE_ALARM:
            // unlock is already handled above
            break;

        case STATE_WINDOW_ACTION:
            if(events & SM_EVENT_WINDOW_TIMEOUT)
            {
                Relay_Window_Off();
                Buzzer_Timed(BUZZER_WINDOW_MS);
                EnterOpenState();
                return;
            }
            break;
    }
}

void StateMachine_Init(void)
{
    pending_events = 0U;
    state = STATE_OPEN;
    motion_count = 0U;
    motion_latched = 0U;
    led_phase_on = 0U;
    alarm_buzzer_on = 0U;

    EnterOpenState();
}

void StateMachine_ProcessEvents(void)
{
    uint32_t events;

    do
    {
        events = StateMachine_FetchEvents();
        if(events != 0U)
            HandleEvents(events);
    } while(events != 0U);
}
