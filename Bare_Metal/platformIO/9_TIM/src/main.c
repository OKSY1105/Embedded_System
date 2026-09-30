 #include "gpio.h"
#include "tim.h"

int main(void)
{
	led_init();
	time_1hz_init();
	while(1)
	{
		led_toggle();
		while(!(TIM2->SR &SR_UIF)){}
		TIM2 -> SR &=~SR_UIF;
	}
}

