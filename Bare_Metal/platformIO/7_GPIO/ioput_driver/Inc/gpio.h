#include <stdbool.h>

#ifndef GPIO_H_
#define GPIO_H_

#include "stm32f4xx.h"

void button_init(void);
bool get_btn_state(void);
void led_init(void);
void led_on(void);
void led_off(void);

#endif