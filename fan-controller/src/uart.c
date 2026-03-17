#include "stm32f103x6.h"
#include "uart.h"
#include "events.h"

static char speedBuffer[3];
static uint8_t speedLen;
static uint8_t waitingSpeedValue;

static uint8_t IsDigit(char c)
{
    return (c >= '0') && (c <= '9');
}

static uint8_t ParseSpeedPercent(const char *buf, uint8_t len, uint8_t *value)
{
    uint16_t parsed = 0U;

    if (len == 0U)
        return 0U;

    for (uint8_t i = 0U; i < len; ++i)
        parsed = (uint16_t)(parsed * 10U + (uint16_t)(buf[i] - '0'));

    if (parsed > 100U)
        return 0U;

    *value = (uint8_t)parsed;
    return 1U;
}

void UART_Init()
{
    const uint32_t baudrate = 9600U;

    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // PA9 -> USART1_TX (AF push-pull), PA10 -> USART1_RX (input floating). 
    GPIOA->CRH &= ~((0xFU << 4) | (0xFU << 8));
    GPIOA->CRH |= (0xBU << 4);
    GPIOA->CRH |= (0x4U << 8);

    // BRR = round(PCLK2 / baudrate) for oversampling by 16. 
    USART1->BRR = (SystemCoreClock + (baudrate / 2U)) / baudrate;

    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;
    NVIC_EnableIRQ(USART1_IRQn);
}

void UART_SendChar(char c)
{
    while(!(USART1->SR & USART_SR_TXE));
    USART1->DR = c;
}

void UART_SendString(const char *s)
{
    while(*s)
        UART_SendChar(*s++);
}

void USART1_IRQHandler(void)
{
    if ((USART1->SR & USART_SR_RXNE) == 0U)
        return;

    char c = (char)(USART1->DR & 0xFFU);

    if (c == '\r')
        UART_SendString("\r\n");
    else
        UART_SendChar(c);

    if (waitingSpeedValue)
    {
        if (IsDigit(c) && (speedLen < sizeof(speedBuffer)))
        {
            speedBuffer[speedLen++] = c;
            return;
        }

        if ((c == '\r') || (c == '\n'))
        {
            uint8_t speedPercent;

            if (ParseSpeedPercent(speedBuffer, speedLen, &speedPercent))
                Events_Push(EVENT_UART_COMMAND, UART_CMD_PACK(UART_CMD_SET_SPEED, speedPercent));
        }

        waitingSpeedValue = 0U;
        speedLen = 0U;
        return;
    }

    switch (c)
    {
        case 'M':
        case 'm':
            Events_Push(EVENT_UART_COMMAND, UART_CMD_PACK(UART_CMD_MODE_MANUAL, 0U));
            UART_SendString("[CMD:MANUAL]\r\n");
        break;

        case 'A':
        case 'a':
            Events_Push(EVENT_UART_COMMAND, UART_CMD_PACK(UART_CMD_MODE_AUTO, 0U));
            UART_SendString("[CMD:AUTO]\r\n");
        break;

        case 'S':
        case 's':
            Events_Push(EVENT_UART_COMMAND, UART_CMD_PACK(UART_CMD_STATUS, 0U));
            UART_SendString("[CMD:STATUS]\r\n");
        break;

        case 'F':
        case 'f':
            waitingSpeedValue = 1U;
            speedLen = 0U;
            UART_SendString("[CMD:FAN %]\r\n");
        break;

        default:
        break;
    }
}

static void UART_SendNumber(int value)
{
    char buffer[10];
    int i = 0;

    if (value == 0)
    {
        UART_SendChar('0');
        return;
    }

    while (value > 0)
    {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i--)
        UART_SendChar(buffer[i]);
}

void UART_PrintStatus(uint8_t mode, uint8_t speed, uint16_t temp, uint16_t potRaw)
{
    UART_SendString("MODE:");

    if (mode == 0)
        UART_SendString("MANUAL ");
    else if (mode == 1)
        UART_SendString("AUTO ");
    else
        UART_SendString("WARNING ");

    UART_SendString("TEMP:");
    UART_SendNumber(temp);

    UART_SendString(" SPEED:");
    UART_SendNumber(speed);

    UART_SendString(" POT_RAW:");
    UART_SendNumber(potRaw);

    UART_SendString(" POT_%:");
    UART_SendNumber((potRaw * 100U) / 4095U);

    UART_SendString("\r\n");
}
