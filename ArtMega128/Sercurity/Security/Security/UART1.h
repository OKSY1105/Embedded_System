/* uart_frame.h */
#ifndef UART_FRAME_H
#define UART_FRAME_H

#include <stdint.h>

/* 프레임 상수 */
#define UART_FRAME_SOF          0xAA
#define UART_FRAME_MAX_PAYLOAD  28
#define UART_FRAME_HEADER_SIZE  3   /* SOF + TYPE + LENGTH */
#define UART_FRAME_TOTAL_MAX    (UART_FRAME_HEADER_SIZE + UART_FRAME_MAX_PAYLOAD + 1)  /* +1: CRC8 */

/* TYPE 정의 */
#define UART_TYPE_DATA  0x01
#define UART_TYPE_CMD   0x02
#define UART_TYPE_ACK   0x03
#define UART_TYPE_ERR   0x04

/* 프레임 구조체 */
typedef struct {
	uint8_t type;
	uint8_t length;
	uint8_t payload[UART_FRAME_MAX_PAYLOAD];
} UartFrame;

/* 함수 선언 */
uint8_t  uart_crc8(uint8_t type, uint8_t length, const uint8_t *payload);
void     uart_send_frame(const UartFrame *frame);
uint8_t  uart_parse_byte(uint8_t byte, UartFrame *out_frame);

#endif /* UART_FRAME_H */
