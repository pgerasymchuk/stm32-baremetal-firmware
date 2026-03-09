#ifndef HARDWARE_H
#define HARDWARE_H

#include "stm32f103x6.h"

#define BTN_LOCK_PORT GPIOA
#define BTN_LOCK_PIN 0

#define BTN_UNLOCK_PORT GPIOA
#define BTN_UNLOCK_PIN 1

#define BTN_ACTION_PORT GPIOA
#define BTN_ACTION_PIN 2


#define MOTION_PORT GPIOA
#define MOTION_PIN 7


#define RELAY_LOCK_PORT GPIOB
#define RELAY_LOCK_PIN 0

#define RELAY_WINDOW_PORT GPIOB
#define RELAY_WINDOW_PIN 1


#define BUZZER_PORT GPIOB
#define BUZZER_PIN 4


#define LED_R_PORT GPIOB
#define LED_R_PIN 5

#define LED_G_PORT GPIOB
#define LED_G_PIN 6

#define LED_B_PORT GPIOB
#define LED_B_PIN 7

#endif