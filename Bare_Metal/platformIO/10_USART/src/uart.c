#include <stdint.h>
#include "uart.h"

#define GPIODEN       (1U << 3)
#define USART3EN      (1U << 18)

#define UART_BAUDRATE 115200
#define SYS_FREQ      16000000
#define APB1_CLK      SYS_FREQ
#define CR1_TE        (1U << 3)
#define CR1_UE        (1U << 13)
#define SR_TXE        (1U << 7)

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate);
static void uart_write(int ch);

int __io_putchar(int ch)
{
    uart_write(ch);
    return ch;
}

int _write(int file, char *buf, int len)
{
    (void)file;

    for (int i = 0; i < len; i++) {
        uart_write((unsigned char)buf[i]);
    }

    return len;
}

void uart_init(void)
{
    /* GPIOD 클록 활성화 */
    RCC->AHB1ENR |= GPIODEN;

    /* PD8을 Alternate Function 모드(10)로 설정 */
    GPIOD->MODER &= ~(3U << 16);
    GPIOD->MODER |=  (2U << 16);

    /* PD8에 AF7(USART3_TX) 선택 */
    GPIOD->AFR[1] &= ~(0xFU << 0);
    GPIOD->AFR[1] |=  (7U << 0);

    /* USART3 클록 활성화 */
    RCC->APB1ENR |= USART3EN;

    uart_set_baudrate(APB1_CLK, UART_BAUDRATE);

    USART3->CR1 |= CR1_TE;
    USART3->CR1 |= CR1_UE;
}

static void uart_write(int ch)
{
    while (!(USART3->SR & SR_TXE)) {}

    USART3->DR = (ch & 0xFF);
}

static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baudrate)
{
    return ((periph_clk + (baudrate / 2U)) / baudrate);
}

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate)
{
    USART3->BRR = compute_uart_bd(periph_clk, baudrate);
}