#include "tim.h"
#define TIM2EN  (1U<<0)
#define CR1_EN  (1U<<0)

void time_1hz_init(void){
    RCC ->APB1ENR |=TIM2EN;
    TIM2 ->PSC = 16000-1;
    TIM2 ->ARR = 1000-1;
    TIM2->CNT = 0 ;

    TIM2 -> CR1 = CR1_EN;
}