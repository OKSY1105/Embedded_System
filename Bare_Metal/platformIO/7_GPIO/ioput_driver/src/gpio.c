#include "gpio.h"


#define GPIOBEN (1U<<1)
#define GPIOCEN (1U<<2)
#define LED_BS7 (1U<<7)
#define LED_BR7 (1U<<23)
#define BTN_PIN (1U<<13)

void button_init(void){
    RCC->AHB1ENR |= GPIOCEN;
    /*Set PA5 mode to output mode*/ 
    GPIOC->MODER &=~(1U<<26); 
    GPIOC->MODER &=~(1U<<27);
}
bool get_btn_state(void){
    if(GPIOC->IDR & BTN_PIN){
        return false;
    }
        
    
    else {
        return true;
    } 
}
void led_init(void)
{
    /*Enable clock access to GPIOA*/ 
    RCC->AHB1ENR |= GPIOBEN;
    /*Set PA5 mode to output mode*/ 
    GPIOB->MODER |=(1U<<14); 
    GPIOB->MODER &=~(1U<<15);
}
void led_on(void)
{
    /*Set PA5 high*/
     GPIOB->BSRR |=LED_BS7;
}
void led_off(void)
{
    /*Set PA5 high*/
     GPIOB->BSRR |=LED_BR7;
}