#include "stm32f103x6.h"
#include "adc.h"
#include "events.h"

#define ADC_CHANNEL_POT   2U
#define ADC_CHANNEL_TEMP  3U

// minimal change (in ADC counts) that is reported to the main loop
#define ADC_CHANGE_THRESHOLD  8U

static uint8_t channel = ADC_CHANNEL_POT;

static volatile uint16_t potRaw = 0xFFFFU;
static volatile uint16_t tempRaw = 0xFFFFU;

static uint8_t ADC_UpdateValue(volatile uint16_t *last, uint16_t value)
{
    uint16_t diff = (value > *last) ? (value - *last) : (*last - value);

    if (diff < ADC_CHANGE_THRESHOLD)
        return 0U;

    *last = value;
    return 1U;
}

static void ADC_StartSingleConversion(uint8_t ch)
{
    ADC1->SQR3 = ch;

    ADC1->CR2 &= ~(7U << 17);
    ADC1->CR2 |= (7U << 17);
    ADC1->CR2 |= (1U << 20);
    ADC1->CR2 |= (1U << 22);
}

static void ADC_WaitBitClear(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = 1000000U;

    while (((*reg) & mask) && timeout)
        timeout--;
}

void ADC_Init()
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    RCC->CFGR &= ~(3U << 14);
    RCC->CFGR |=  (2U << 14);

    // PA2 / PA3 as analog inputs (potentiometer and temperature)
    GPIOA->CRL &= ~((0xFU << 8) | (0xFU << 12));

    ADC1->SMPR2 &= ~((7U << (3U * 2U)) | (7U << (3U * 3U)));
    ADC1->SMPR2 |=  ((5U << (3U * 2U)) | (5U << (3U * 3U)));

    ADC1->SQR1 = 0;
    ADC1->SQR2 = 0;
    ADC1->CR2 |= ADC_CR2_ADON;

    ADC1->CR2 |= (1U << 3);
    ADC_WaitBitClear(&ADC1->CR2, (1U << 3));

    ADC1->CR2 |= (1U << 2);
    ADC_WaitBitClear(&ADC1->CR2, (1U << 2));

    ADC1->CR1 |= ADC_CR1_EOCIE;

    NVIC_EnableIRQ(ADC1_2_IRQn);

    ADC_StartSingleConversion(channel);
}

void ADC1_2_IRQHandler()
{
    if ((ADC1->SR & ADC_SR_EOC) == 0U)
        return;

    uint16_t value = ADC1->DR;

    if (channel == ADC_CHANNEL_POT)
    {
        if (ADC_UpdateValue(&potRaw, value))
            Events_Push(EVENT_POT_CHANGED, value);
        channel = ADC_CHANNEL_TEMP;
    }
    else
    {
        if (ADC_UpdateValue(&tempRaw, value))
            Events_Push(EVENT_TEMP_CHANGED, value);
        channel = ADC_CHANNEL_POT;
    }

    ADC_StartSingleConversion(channel);
}

uint16_t ADC_GetPotRaw(void)
{
    return potRaw;
}

uint16_t ADC_GetTempRaw(void)
{
    return tempRaw;
}
