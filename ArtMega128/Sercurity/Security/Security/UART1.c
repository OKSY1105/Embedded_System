#include "UART1.h"

// main.c에 있는 1바이트 송신 함수 가져오기 (대소문자 맞춤)
extern void UART1_putc(char data);

// ==========================================
// [명세서 5.2] CRC8 계산 함수 (필수!)
// ==========================================
uint8_t uart_crc8(uint8_t type, uint8_t length, const uint8_t *payload) {
	uint8_t crc = type ^ length;
	for (uint8_t i = 0; i < length; i++) {
		crc ^= payload[i];
	}
	return crc;
}

// ==========================================
// [명세서 5.3] 프레임 송신 함수
// ==========================================
void uart_send_frame(const UartFrame *frame) {
	uint8_t crc = uart_crc8(frame->type, frame->length, frame->payload);

	// 대소문자 맞춘 UART1_putc 사용
	UART1_putc(UART_FRAME_SOF);
	UART1_putc(frame->type);
	UART1_putc(frame->length);
	for (uint8_t i = 0; i < frame->length; i++) {
		UART1_putc(frame->payload[i]);
	}
	UART1_putc(crc);
}

// ==========================================
// [명세서 5.4] 프레임 수신 파서 (상태 머신)
// ==========================================
// 수신 상태 열거형
typedef enum {
	RX_WAIT_SOF,
	RX_WAIT_TYPE,
	RX_WAIT_LENGTH,
	RX_WAIT_PAYLOAD,
	RX_WAIT_CRC
} RxState;

uint8_t uart_parse_byte(uint8_t byte, UartFrame *out) {
	static RxState  state      = RX_WAIT_SOF;
	static UartFrame rx_buf;
	static uint8_t  pay_idx   = 0;

	switch (state) {
		case RX_WAIT_SOF:
		if (byte == UART_FRAME_SOF) state = RX_WAIT_TYPE;
		break;

		case RX_WAIT_TYPE:
		rx_buf.type = byte;
		state = RX_WAIT_LENGTH;
		break;

		case RX_WAIT_LENGTH:
		if (byte > UART_FRAME_MAX_PAYLOAD) {
			state = RX_WAIT_SOF;  /* 범위 초과 → 재동기화 */
			} else {
			rx_buf.length = byte;
			pay_idx = 0;
			state = (byte == 0) ? RX_WAIT_CRC : RX_WAIT_PAYLOAD;
		}
		break;

		case RX_WAIT_PAYLOAD:
		rx_buf.payload[pay_idx++] = byte;
		if (pay_idx >= rx_buf.length) state = RX_WAIT_CRC;
		break;

		case RX_WAIT_CRC: {
			uint8_t expected = uart_crc8(rx_buf.type, rx_buf.length, rx_buf.payload);
			state = RX_WAIT_SOF;
			if (byte == expected) {
				*out = rx_buf;
				return 1;  /* 프레임 완성 */
			}
			/* CRC 불일치 → ERR 응답 */
			UartFrame err = { UART_TYPE_ERR, 5, {'E',':','C','R','C'} };
			uart_send_frame(&err);
			break;
		}
	}
	return 0;
}