#include "systick.h"
#define CTRL_ENABLE     (1U<<0)
#define CTRL_CLCKSRC        (1U<<2)
#define CTRL_COUNTFLAG  (1U<<16)

#define ONE_MSEC_LOAD   16000

void systick_mesc_delay(uint32_t delay){
    SysTick -> LOAD = ONE_MSEC_LOAD-1;
    SysTick ->VAL = 0;
    SysTick ->CTRL = CTRL_CLCKSRC;
    SysTick ->CTRL = CTRL_ENABLE;

    for(int i=0; i < delay; i++){
        while((SysTick->CTRL & CTRL_COUNTFLAG )==0){}
    }
    SysTick->CTRL=0;
}