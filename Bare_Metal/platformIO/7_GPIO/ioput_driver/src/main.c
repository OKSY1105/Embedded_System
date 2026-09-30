 #include "gpio.h"
bool btn_state;

int main(void)
{
	led_init();
    button_init();
	while(1)
	{
        btn_state = get_btn_state();
		
		if (btn_state)
		{
    		led_on();
		}
        
        else {
            led_off();
        }
	}
}

/*#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830U)
#define GPIOB_MODER  (*(volatile uint32_t *)0x40020400U)
#define GPIOB_BSRR   (*(volatile uint32_t *)0x40020418U)

int main(void)
{
    RCC_AHB1ENR |= (1U << 1);     // GPIOB 클럭
    (void)RCC_AHB1ENR;

    GPIOB_MODER &= ~3U;           // PB0의 모드 비트 [1:0] 지우기
    GPIOB_MODER |=  1U;           // PB0 출력
    GPIOB_BSRR = 1U;              // PB0 HIGH

    while (1) {
    }
}*/

/*#include "stm32f4xx.h"

int main(void)
{
    RCC->AHB1ENR |= (1U << 1);
    (void)RCC->AHB1ENR;

    GPIOB->MODER = (GPIOB->MODER & ~3U) | 1U;
    GPIOB->BSRR = 1U;                 // PB0 HIGH

    while (1) {
    }
}*/
