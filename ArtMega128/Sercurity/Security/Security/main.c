#define F_CPU 16000000UL
#define BAUD_RATE 9600

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>        // [추가] strncmp 문자열 비교 함수 사용
#include <avr/interrupt.h>
#include "UART1.h"

const uint8_t fnd[17]={0xc0, 0xf9,0xa4,0xb0,0x99,0x92,0x82,0xd8,0x80,0x98,0x88,0x83,0xc6,0xa1,0x86,0x8e,0x7f};

void UART1_init(void) {
	uint16_t ubrr_value = (F_CPU / (16UL * BAUD_RATE)) - 1;
	UBRR1H = (unsigned char)(ubrr_value >> 8);
	UBRR1L = (unsigned char)ubrr_value;
	UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
}

void UART1_putc(char data) {
	while (!(UCSR1A & (1 << UDRE1)));
	UDR1 = data;
}

void send_sensor_data(const char *tag, uint16_t val) {
	UartFrame f;
	f.type = UART_TYPE_DATA;

	uint8_t tag_len = strlen(tag);
	// "태그" + ':' + 4자리 숫자 = tag_len + 5 바이트가 필요.
	// UART_FRAME_MAX_PAYLOAD(28바이트)를 넘으면 안 되므로 방어 코드 추가.
	if (tag_len + 5 > UART_FRAME_MAX_PAYLOAD) {
		return;  // 태그가 너무 길면 전송하지 않음 (버퍼 오버플로 방지)
	}

	uint8_t idx = 0;
	for (uint8_t i = 0; i < tag_len; i++) {
		f.payload[idx++] = tag[i];
	}
	f.payload[idx++] = ':';
	f.payload[idx++] = '0' + ((val / 1000) % 10);
	f.payload[idx++] = '0' + ((val /  100) % 10);
	f.payload[idx++] = '0' + ((val /   10) % 10);
	f.payload[idx++] = '0' + ( val         % 10);

	f.length = idx;
	uart_send_frame(&f);
}

void send_msg_data(const char *msg) {
	UartFrame f;
	f.type = UART_TYPE_DATA;
	uint8_t len = 0;
	while (msg[len] != '\0' && len < UART_FRAME_MAX_PAYLOAD) {
		f.payload[len] = msg[len];
		len++;
	}
	f.length = len;
	uart_send_frame(&f);
}

volatile uint8_t  g_frame_ready = 0;
volatile UartFrame g_rx_frame;

ISR(USART1_RX_vect) {
	uint8_t byte = UDR1;
	UartFrame tmp;
	if (uart_parse_byte(byte, &tmp)) {
		g_rx_frame = tmp;
		g_frame_ready = 1;
	}
}

void ADC_init(void) {
	ADMUX = 0x00;
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t read_ADC(uint8_t channel) {
	ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

int main(void)
{
	PORTF = 0xF8;
	DDRF = 0x05;
	
	PORTC = 0x0f;
	DDRC = 0xff;
	
	DDRA = 0xff;
	PORTA = 0xff;
	
	DDRE &= ~(1 << PE4);
	DDRE |= (1 << PE3);
	PORTE &= ~(1 << PE3);
	
	DDRD |= (1 << PD3);
	DDRB = 0xBF;
	PORTB = 0x80;

	UART1_init();
	ADC_init();
	sei();
	
	uint8_t h = 0, m = 0;
	uint16_t loop_cnt = 0;
	uint16_t uart_cnt = 0;
	uint16_t blink_cnt = 0;

	uint8_t mode = 0;
	uint8_t alert_flag = 0;
	uint8_t prev_pf3 = 1;
	uint8_t led_state = 0x0F;

	uint8_t pw_step = 0;
	uint8_t pw_sw_pressed = 0;

	uint16_t echo_time = 0;
	uint16_t distance_cm = 0;
	uint8_t threshold_cm = 15;

	send_msg_data("SYS:START");

	while (1)
	{
		// ==========================================
		// [수정됨] PC(Python)에서 들어온 명령(CMD) 처리
		// ==========================================
		if (g_frame_ready) {
			g_frame_ready = 0;
			UartFrame f = g_rx_frame;
			
			if (f.type == UART_TYPE_CMD) {
				// 페이로드 길이가 4이고 내용이 "stop" 또는 "STOP"인 경우
				if (f.length == 4 && (strncmp((char*)f.payload, "stop", 4) == 0 || strncmp((char*)f.payload, "STOP", 4) == 0)) {
					// 경계 모드(mode == 1)일 때만 해제 동작 수행
					if (mode == 1) {
						mode = 0;          // 일반 모드로 복귀
						pw_step = 0;       // 비밀번호 진행 단계 초기화
						alert_flag = 0;    // LED 및 부저, 모터 알람 즉시 정지
						
						// PC 명령으로 원격 해제되었음을 알리는 메시지 전송
						send_msg_data("PW:OK_PC");
					}
				}

				// 명령 처리 후 정상 수신(ACK) 응답 전송
				UartFrame ack = { UART_TYPE_ACK, 2, {'O','K'} };
				uart_send_frame(&ack);
			}
		}

		loop_cnt++;
		uart_cnt++;

		// ==========================================
		// 1. PF3 스위치 (1->0 경계 진입 / 0->1 일반 복귀)
		// ==========================================
		uint8_t current_pf3 = (PINF & (1 << PF3)) ? 1 : 0;
		
		if (prev_pf3 == 1 && current_pf3 == 0) {
			mode = 1;
			alert_flag = 0;
			pw_step = 0;
			send_msg_data("MD:GUARD");
		}
		else if (prev_pf3 == 0 && current_pf3 == 1) {
			mode = 0;
			alert_flag = 0;
			pw_step = 0;
			send_msg_data("MD:NORMAL");
		}
		
		prev_pf3 = current_pf3;

		// ==========================================
		// 2. 물리적 PF4~PF7 스위치 (경계 모드 알람 해제)
		// ==========================================
		if (mode == 1) {
			uint8_t current_sw = 0;
			if (!(PINF & (1 << PF4))) current_sw = 4;
			else if (!(PINF & (1 << PF5))) current_sw = 5;
			else if (!(PINF & (1 << PF6))) current_sw = 6;
			else if (!(PINF & (1 << PF7))) current_sw = 7;

			if (current_sw != 0) {
				if (pw_sw_pressed == 0) {
					pw_sw_pressed = 1;
					
					if (current_sw == 4 && pw_step == 0) { pw_step = 1; send_msg_data("PW:STEP1"); }
					else if (current_sw == 5 && pw_step == 1) { pw_step = 2; send_msg_data("PW:STEP2"); }
					else if (current_sw == 6 && pw_step == 2) { pw_step = 3; send_msg_data("PW:STEP3"); }
					else if (current_sw == 7 && pw_step == 3) {
						mode = 0;
						pw_step = 0;
						alert_flag = 0;
						send_msg_data("PW:OK_NRM");
						} else {
						pw_step = 0;
						send_msg_data("PW:ERROR");
					}
				}
				} else {
				pw_sw_pressed = 0;
			}
		}

		// ==========================================
		// 3. 타이머 (시계 기능)
		// ==========================================
		if (loop_cnt >= 750) {
			loop_cnt = 0;
			m++;
			if (m >= 60) { m = 0; h++; }
			if (h >= 24) { h = 0; }
		}

		// ==========================================
		// 4. 초음파 측정 및 ADC 임계값 갱신 (약 1초)
		// ==========================================
		if (uart_cnt >= 125) {
			uart_cnt = 0;
			
			if (mode == 1) {
				uint16_t adc_val = read_ADC(1);
				threshold_cm = 5 + (adc_val / 20);

				PORTE &= ~(1 << PE3); _delay_us(2);
				PORTE |= (1 << PE3);  _delay_us(10);
				PORTE &= ~(1 << PE3);

				uint16_t timeout = 0;
				uint8_t error_flag = 0;

				while(!(PINE & (1 << PE4))) {
					_delay_us(10);
					timeout++;
					if (timeout > 1000) { error_flag = 1; break; }
				}

				if (error_flag == 0) {
					TCNT3 = 0;
					TCCR3B = (1 << CS31);
					timeout = 0;
					
					while(PINE & (1 << PE4)) {
						_delay_us(10);
						timeout++;
						if (timeout > 3000) { error_flag = 2; break; }
					}
					TCCR3B = 0;

					if (error_flag == 0) {
						echo_time = TCNT3 / 2;
						distance_cm = echo_time / 58;
						
						send_sensor_data("Distance", distance_cm);
						send_sensor_data("Threshold",threshold_cm);
						
						if (distance_cm <= threshold_cm) alert_flag = 1;
						else alert_flag = 0;
						
						} else {
						alert_flag = 0;
						send_msg_data("ERR:RANGE");
					}
					} else {
					alert_flag = 0;
					send_msg_data("ERR:SONAR");
				}
				} else {
				alert_flag = 0;
			}
		}

		// ==========================================
		// 5. 알람 동작 (모터 구동, LED 깜빡임, 부저 울림)
		// ==========================================
		if (alert_flag == 1 && mode == 1) {
			uint8_t temp_portb = (1 << PB5) | (1 << PB4) | (1 << PB2) | (1 << PB0);
			
			blink_cnt++;
			if (blink_cnt >= 30) {
				blink_cnt = 0;
				if (led_state == 0x0F) led_state = 0x00;
				else led_state = 0x0F;
			}
			
			if (led_state == 0x00) temp_portb |= (1 << PB7);
			
			PORTB = temp_portb;
			} else {
			PORTB = 0x00;
			led_state = 0x0F;
		}
		
		// ==========================================
		// 6. FND 다이나믹 구동 (8ms)
		// ==========================================
		uint8_t disp[4] = { m % 10, m / 10, h % 10, h / 10 };
		for (uint8_t i = 0; i < 4; i++) {
			PORTC = led_state | 0xF0;
			PORTA = fnd[disp[i]];
			PORTC = (led_state & 0x0F) | (~(1 << (i + 4)) & 0xF0);
			_delay_ms(2);
		}
	}
}