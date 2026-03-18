#include "state_machine.h"
#include "events.h"
#include "pwm.h"
#include "uart.h"

#define POT_ADC_MAX            4095U
#define TEMP_WARNING_C         60U
#define TEMP_WARNING_HYST_C    55U
#define POT_UART_TAKEOVER_RAW  50U

typedef enum
{
    MODE_MANUAL,
    MODE_AUTO,
    MODE_WARNING
} Mode;

typedef enum
{
    MANUAL_SRC_POT = 0,
    MANUAL_SRC_UART
} ManualSource;

static Mode mode;
static Mode modeBeforeWarning;
static ManualSource manualSource;

static uint16_t temperature;
static uint16_t pot;
static uint16_t potAtLastUartSet;
static uint8_t speed;

static uint8_t PotRawToPercent(uint16_t raw)
{
    return (uint8_t)((raw * 100U) / POT_ADC_MAX);
}

// LM35 conversion: temperature(*C) = (ADC * 3300mV / 4095) / 10mV
static uint16_t TempRawToC(uint16_t raw)
{
    return (uint16_t)(((uint32_t)raw * 330U) / POT_ADC_MAX);
}

static void SetSpeed(uint8_t s)
{
    if (s > 100U)
        s = 100U;

    speed = s;

    PWM_SetFan(s);
    PWM_SetLED(s);
}

static uint16_t AbsDiffU16(uint16_t a, uint16_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

static void AutomaticControl()
{
    if (temperature < 30)
        SetSpeed(30);
    else if (temperature < 40)
        SetSpeed(60);
    else
        SetSpeed(100);
}

static void ApplyModeSpeed()
{
    if (mode == MODE_AUTO)
        AutomaticControl();
    else if (mode == MODE_MANUAL)
        SetSpeed(PotRawToPercent(pot));
    else
        SetSpeed(100);
}

static void CheckTemperature()
{
    if (temperature >= TEMP_WARNING_C)
    {
        if (mode != MODE_WARNING)
        {
            modeBeforeWarning = mode;
            UART_SendString("WARNING: OVERHEAT\r\n");
        }

        mode = MODE_WARNING;
        SetSpeed(100);
    }
    else if ((mode == MODE_WARNING) && (temperature <= TEMP_WARNING_HYST_C))
    {
        mode = modeBeforeWarning;
        ApplyModeSpeed();
    }

    if (mode == MODE_AUTO)
        AutomaticControl();
}

void StateMachine_Init()
{
    mode = MODE_MANUAL;
    modeBeforeWarning = MODE_MANUAL;
    manualSource = MANUAL_SRC_POT;
    temperature = 0U;
    pot = 0U;
    potAtLastUartSet = 0U;

    SetSpeed(0U);
    UART_SendString("SYSTEM READY\r\n");
}

void StateMachine_ProcessEvents()
{
    Event e;

    while (Events_Pop(&e))
    {
        switch (e.type)
        {
            case EVENT_BUTTON_MODE:

                if (mode == MODE_WARNING)
                {
                    mode = modeBeforeWarning;
                }
                else if (mode == MODE_MANUAL)
                {
                    mode = MODE_AUTO;
                    modeBeforeWarning = MODE_AUTO;
                }
                else
                {
                    mode = MODE_MANUAL;
                    modeBeforeWarning = MODE_MANUAL;
                }

                manualSource = MANUAL_SRC_POT;
                ApplyModeSpeed();
                CheckTemperature();

            break;

            case EVENT_POT_CHANGED:

                pot = e.data;

                if (mode == MODE_MANUAL)
                {
                    if (manualSource == MANUAL_SRC_POT)
                    {
                        SetSpeed(PotRawToPercent(pot));
                    }
                    else if (AbsDiffU16(pot, potAtLastUartSet) >= POT_UART_TAKEOVER_RAW)
                    {
                        manualSource = MANUAL_SRC_POT;
                        SetSpeed(PotRawToPercent(pot));
                    }
                }

            break;

            case EVENT_TEMP_CHANGED:

                temperature = TempRawToC(e.data);
                CheckTemperature();

            break;

            case EVENT_STATUS_TIMER:

                UART_PrintStatus(mode, speed, temperature, pot);

            break;

            case EVENT_UART_COMMAND:
            {
                uint8_t cmd = UART_CMD_ID(e.data);
                uint8_t value = UART_CMD_VALUE(e.data);

                switch (cmd)
                {
                    case UART_CMD_MODE_MANUAL:
                        mode = MODE_MANUAL;
                        modeBeforeWarning = MODE_MANUAL;
                        manualSource = MANUAL_SRC_POT;
                        SetSpeed(PotRawToPercent(pot));
                    break;

                    case UART_CMD_MODE_AUTO:
                        mode = MODE_AUTO;
                        modeBeforeWarning = MODE_AUTO;
                        AutomaticControl();
                    break;

                    case UART_CMD_SET_SPEED:
                        if (mode == MODE_MANUAL)
                        {
                            manualSource = MANUAL_SRC_UART;
                            potAtLastUartSet = pot;
                            SetSpeed(value);
                        }
                    break;

                    case UART_CMD_STATUS:
                        UART_PrintStatus(mode, speed, temperature, pot);
                    break;

                    default:
                    break;
                }
            }
            break;

            default:
            break;
        }
    }
}
