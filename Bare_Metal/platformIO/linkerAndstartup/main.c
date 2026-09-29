#define peripherals_base  (0x40000000UL)

#define ahb1_offset (0x00020000UL)

#define ahb1_base (peripherals_base + ahb1_offset)

#define gpioB_start (0x0400UL)

#define gpioB_base (ahb1_base + gpioB_start)

#define RCC_offset (0x3800UL)

#define RCC_base (ahb1_base + RCC_offset)

#define RCC_AHB1_EN_offset (0x30UL)

#define gpioBen (*(volatile unsigned int *) (RCC_base + RCC_AHB1_EN_offset))

#define MODDER_offset (0x00UL)

#define gpioB_MODDER_offset (*(volatile unsigned int *) (gpioB_base + MODDER_offset))

#define output_offset (0x14UL)

#define gpioB_output_offset (*(volatile unsigned int*) (gpioB_base + output_offset))

#define PIN0 (1U << 0)

#define LED_PIN PIN0

int main(void) {

	gpioBen = 1U<<1;

	gpioB_MODDER_offset |= (1U << 0);
	gpioB_MODDER_offset &= ~(1U << 1);

	while(1){
		gpioB_output_offset ^= LED_PIN;
		for(int i =0; i<500000; i++){} //delay loop for blinking

	}
}

