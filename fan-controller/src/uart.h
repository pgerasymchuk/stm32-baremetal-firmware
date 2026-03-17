#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>

typedef enum
{
    UART_CMD_NONE = 0,
    UART_CMD_MODE_MANUAL = 1,
    UART_CMD_MODE_AUTO = 2,
    UART_CMD_SET_SPEED = 3,
    UART_CMD_STATUS = 4
} UART_Command;

#define UART_CMD_PACK(cmd, value)  ((((uint16_t)(cmd)) << 8) | (uint16_t)(value))
#define UART_CMD_ID(data)          ((uint8_t)(((uint16_t)(data) >> 8) & 0xFFU))
#define UART_CMD_VALUE(data)       ((uint8_t)((uint16_t)(data) & 0xFFU))

void UART_Init(void);

void UART_SendChar(char c);
void UART_SendString(const char *s);

void UART_PrintStatus(uint8_t mode, uint8_t speed, uint16_t temp, uint16_t potRaw);

#endif
