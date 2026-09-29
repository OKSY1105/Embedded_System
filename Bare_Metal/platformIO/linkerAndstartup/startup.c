#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void);
void NMI_Handler(void);

int main (void) ;

void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));

uint32_t vector_tbl[] __attribute__((section(".isr_vector_tbl"))) = { 
	(uint32_t)&_estack,
	(uint32_t)&Reset_Handler,
	(uint32_t)&NMI_Handler,
	(uint32_t)&HardFault_Handler,
	(uint32_t)&MemManage_Handler,
};

void Default_Handler(void) {
	while(1) {
	}
}

void NMI_Handler(void){
    	while (1) {
    	}
}

void Reset_Handler(void){
	uint32_t data_size = ((uint32_t)&_edata - (uint32_t)&_sdata) / 4;
    	uint32_t bss_size  = ((uint32_t)&_ebss  - (uint32_t)&_sbss)  / 4;

    	uint32_t *src = (uint32_t *)&_etext;
    	uint32_t *dst = (uint32_t *)&_sdata;

    	for (uint32_t i = 0; i < data_size; i++)
        	*dst++ = *src++;

    	dst = (uint32_t *)&_sbss;
    	for (uint32_t i = 0; i < bss_size; i++)
        	*dst++ = 0;

    	main();
    	while (1) {}
}
