 #include "gpio.h"


int main(void)
{
	led_init();
	while(1)
	{
		systick_mesc_delay(500);
        led_toggle();
	}
}

