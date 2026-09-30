#include "gpio.h"

#define GPIOBEN (1U<<1)
#define GPIOCEN (1U<<2)
#define LED_BS0 (1U<<0)
#define LED_BR0 (1U<<16)
#define BTN_PIN (1U<<13)

void led_init(void)
{
    /*Enable clock access to GPIOA*/ 
    RCC->AHB1ENR |= GPIOBEN;
    /*Set PA5 mode to output mode*/ 
    GPIOB->MODER |=(1U<<0); 
    GPIOB->MODER &=~(1U<<1);
}
void led_on(void)
{
    /*Set PA5 high*/
     GPIOB->BSRR |=LED_BS0;
}
void led_off(void)
{
    /*Set PA5 high*/
     GPIOB->BSRR |=LED_BR0;
}