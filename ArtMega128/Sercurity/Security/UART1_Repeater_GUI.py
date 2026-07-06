import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading
import time

# --- [프로토콜 규격 상수] ---
UART_FRAME_SOF = 0xAA
UART_TYPE_DATA = 0x01
UART_TYPE_CMD  = 0x02
UART_TYPE_ACK  = 0x03
UART_TYPE_ERR  = 0x04

# --- [UART 프레임 파서 클래스 (명세서 6.2)] ---
class UartFrameParser:
    def __init__(self):
        self._reset()

    def _reset(self):
        self._state = 'WAIT_SOF'
        self._type = 0
        self._length = 0
        self._payload = bytearray()

    def feed(self, byte: int):
        if self._state == 'WAIT_SOF':
            if byte == UART_FRAME_SOF:
                self._state = 'WAIT_TYPE'
        elif self._state == 'WAIT_TYPE':
            self._type = byte
            self._state = 'WAIT_LENGTH'
        elif self._state == 'WAIT_LENGTH':
            if byte > 28:
                self._reset()
            else:
                self._length = byte
                self._payload = bytearray()
                self._state = 'WAIT_CRC' if byte == 0 else 'WAIT_PAYLOAD'
        elif self._state == 'WAIT_PAYLOAD':
            self._payload.append(byte)
            if len(self._payload) >= self._length:
                self._state = 'WAIT_CRC'
        elif self._state == 'WAIT_CRC':
            expected = self._type ^ self._length
            for b in self._payload:
                expected ^= b
            expected &= 0xFF

            frame = None
            if byte == expected:
                frame = {
                    'type': self._type,
                    'length': self._length,
                    'payload': bytes(self._payload),
                }
            self._reset()
            return frame
        return None

# --- [메인 GUI 애플리케이션] ---
class UartRepeaterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("ATmega128 UART Protocol Repeater")
        self.root.geometry("600x560")

        self.serial_port = None
        self.is_connected = False
        self.parser = UartFrameParser()

        # 알람 상태 관련
        self.is_alarm_active = False
        self.mode_guard = False        # 경계 모드 여부 (MD:GUARD ~ MD:NORMAL 사이)
        self.last_distance = None      # 최근 수신된 D0 값
        self.last_threshold = None     # 최근 수신된 T0 값

        self.create_widgets()
        self.refresh_ports()

    def create_widgets(self):
        # 상단 설정 프레임
        setup_frame = tk.LabelFrame(self.root, text="통신 설정", padx=10, pady=5)
        setup_frame.pack(fill="x", padx=10, pady=5)

        tk.Label(setup_frame, text="COM 포트:").pack(side="left")
        self.cb_ports = ttk.Combobox(setup_frame, width=15)
        self.cb_ports.pack(side="left", padx=5)

        tk.Button(setup_frame, text="새로고침", command=self.refresh_ports).pack(side="left", padx=5)

        self.btn_connect = tk.Button(setup_frame, text="연결", width=10, command=self.toggle_connection, bg="#4CAF50", fg="white")
        self.btn_connect.pack(side="left", padx=10)

        # 알람 제어 프레임 (STOP / ALARM)
        alarm_frame = tk.LabelFrame(self.root, text="알람 제어", padx=10, pady=10)
        alarm_frame.pack(fill="x", padx=10, pady=5)

        self.btn_stop = tk.Button(
            alarm_frame, text="STOP", width=12, height=2,
            command=self.send_stop, bg="#FF9800", fg="white",
            font=("Arial", 11, "bold")
        )
        self.btn_stop.pack(side="left", padx=10)

        self.btn_alarm = tk.Button(
            alarm_frame, text="ALARM", width=12, height=2,
            state="disabled", bg="#FF0000", fg="white",
            disabledforeground="white",
            font=("Arial", 11, "bold")
        )
        # 평소에는 보이지 않다가 알람 발생 시에만 pack()으로 표시한다.

        # 중앙 로그 창
        log_frame = tk.LabelFrame(self.root, text="통신 로그", padx=10, pady=5)
        log_frame.pack(fill="both", expand=True, padx=10, pady=5)

        self.txt_log = scrolledtext.ScrolledText(log_frame, wrap=tk.WORD, height=15)
        self.txt_log.pack(fill="both", expand=True)

        # 하단 수동 송신 프레임
        send_frame = tk.LabelFrame(self.root, text="명령(CMD) 수동 송신", padx=10, pady=10)
        send_frame.pack(fill="x", padx=10, pady=5)

        tk.Label(send_frame, text="입력 (예: stop) :").pack(side="left")
        self.ent_cmd = tk.Entry(send_frame, width=30)
        self.ent_cmd.pack(side="left", padx=5)
        self.ent_cmd.bind("<Return>", lambda event: self.send_cmd())  # 엔터키 전송 지원

        tk.Button(send_frame, text="전송", width=10, command=self.send_cmd, bg="#2196F3", fg="white").pack(side="left", padx=5)

    def log(self, msg, color="black"):
        timestamp = time.strftime("[%H:%M:%S]")
        self.txt_log.insert(tk.END, f"{timestamp} {msg}\n")
        self.txt_log.see(tk.END)

    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        self.cb_ports['values'] = [port.device for port in ports]
        if self.cb_ports['values']:
            self.cb_ports.current(0)

    def toggle_connection(self):
        if not self.is_connected:
            port = self.cb_ports.get()
            if not port:
                messagebox.showwarning("경고", "COM 포트를 선택하세요.")
                return
            try:
                self.serial_port = serial.Serial(port, 9600, timeout=0.1)
                self.is_connected = True
                self.btn_connect.config(text="해제", bg="#F44336")
                self.log(f"{port} 포트에 연결되었습니다.", "blue")
                self.cb_ports.config(state="disabled")

                # 수신 스레드 시작
                self.rx_thread = threading.Thread(target=self.receive_loop, daemon=True)
                self.rx_thread.start()
            except Exception as e:
                messagebox.showerror("오류", f"포트 연결 실패:\n{e}")
        else:
            self.is_connected = False
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
            self.btn_connect.config(text="연결", bg="#4CAF50")
            self.cb_ports.config(state="readonly")
            self.log("연결이 해제되었습니다.", "red")

    def encode_uart_frame(self, frame_type, payload: bytes):
        length = len(payload)
        crc = frame_type ^ length
        for b in payload:
            crc ^= b
        crc &= 0xFF
        return bytes([UART_FRAME_SOF, frame_type, length]) + payload + bytes([crc])

    def _send_text_cmd(self, cmd_text: str):
        """CMD 프레임으로 문자열 송신하는 공통 로직"""
        if not self.is_connected or not self.serial_port:
            messagebox.showwarning("경고", "먼저 포트를 연결하세요.")
            return False

        payload = cmd_text.encode('utf-8')
        if len(payload) > 28:
            messagebox.showwarning("경고", "명령어는 28바이트를 초과할 수 없습니다.")
            return False

        frame_bytes = self.encode_uart_frame(UART_TYPE_CMD, payload)

        try:
            self.serial_port.write(frame_bytes)
            self.log(f"→ [송신] CMD : {cmd_text}")
            return True
        except Exception as e:
            self.log(f"송신 오류: {e}")
            return False

    def send_cmd(self):
        cmd_text = self.ent_cmd.get()
        if not cmd_text:
            return
        if self._send_text_cmd(cmd_text):
            self.ent_cmd.delete(0, tk.END)  # 입력창 지우기

    def send_stop(self):
        """STOP 버튼: 'stop' 문자열을 송신 (펌웨어가 경계 모드였다면 alert_flag/mode를 0으로 되돌림).
        실제 확인은 이어서 수신되는 PW:OK_PC 메시지로도 처리되지만, 여기서도 즉시 반영해둔다."""
        if self._send_text_cmd("stop"):
            self.mode_guard = False
            self.last_distance = None
            self.last_threshold = None
            self.deactivate_alarm()

    # --- [알람 상태 제어] ---
    def activate_alarm(self):
        if self.is_alarm_active:
            return
        self.is_alarm_active = True
        self.log("⚠ 알람 상태 감지 (ALARM 버튼 표시)")
        self.btn_alarm.pack(side="left", padx=10)  # 알람 버튼 나타남

    def deactivate_alarm(self):
        if not self.is_alarm_active:
            return
        self.is_alarm_active = False
        self.btn_alarm.pack_forget()  # 알람 버튼 숨김
        self.log("알람 상태 해제")

    def receive_loop(self):
        while self.is_connected:
            try:
                if self.serial_port.in_waiting:
                    raw_data = self.serial_port.read(self.serial_port.in_waiting)
                    for byte in raw_data:
                        frame = self.parser.feed(byte)
                        if frame:
                            # Tkinter는 메인 스레드에서만 UI를 안전하게 갱신할 수 있으므로
                            # after()를 통해 메인 스레드로 처리를 넘긴다.
                            self.root.after(0, self.process_received_frame, frame)
            except Exception as e:
                if self.is_connected:  # 의도적인 종료가 아닌 에러일 경우
                    self.root.after(0, self.log, f"수신 에러: {e}")
                    self.root.after(0, self.toggle_connection)  # 메인 스레드에서 UI 업데이트 및 해제
                break
            time.sleep(0.01)

    def process_received_frame(self, frame):
        f_type = frame['type']
        payload_str = frame['payload'].decode('utf-8', errors='ignore')

        if f_type == UART_TYPE_DATA:
            self.log(f"← [수신] DATA : {payload_str}")
        elif f_type == UART_TYPE_ACK:
            self.log(f"← [수신] ACK  : {payload_str}")
        elif f_type == UART_TYPE_ERR:
            self.log(f"← [수신] ERR  : {payload_str}")
        else:
            self.log(f"← [수신] UNKNOWN(0x{f_type:02X}) : {payload_str}")

        # 아트메가 펌웨어가 보내는 상태 메시지(MD:*, D0:*, T0:*, PW:OK_*)를 해석하여
        # 알람(경계 모드 + 거리 <= 임계값) 여부를 판단한다.
        self._handle_status_message(payload_str)

    def _handle_status_message(self, payload_str: str):
        if payload_str == "MD:GUARD":
            # 경계 모드 진입. 실제 알람 여부는 이후 D0/T0 값 비교로 판단.
            self.mode_guard = True

        elif payload_str == "MD:NORMAL":
            # PF3 스위치로 일반 모드 복귀 -> 알람 강제 해제
            self.mode_guard = False
            self.last_distance = None
            self.last_threshold = None
            self.deactivate_alarm()

        elif payload_str in ("PW:OK_PC", "PW:OK_NRM"):
            # 비밀번호(물리 스위치) 또는 PC 명령으로 해제 -> 펌웨어에서도 mode=0, alert_flag=0 처리됨
            self.mode_guard = False
            self.last_distance = None
            self.last_threshold = None
            self.deactivate_alarm()

        elif ':' in payload_str:
            # "Distance0:0056", "Threshold0:0056" 형태의 센서 값 처리
            # 태그 길이가 가변적이므로 인덱스로 자르지 말고 ':' 기준으로 분리한다.
            tag, _, value_str = payload_str.partition(':')
            if value_str.isdigit():
                value = int(value_str)
                if tag == "Distance":
                    self.last_distance = value
                    self._check_alarm_condition()
                elif tag == "Threshold":
                    self.last_threshold = value
                    self._check_alarm_condition()

    def _check_alarm_condition(self):
        """경계 모드이고 거리값이 임계값 이하이면 알람 ON, 아니면 OFF (펌웨어 alert_flag 로직과 동일)"""
        if not self.mode_guard or self.last_distance is None or self.last_threshold is None:
            return
        if self.last_distance <= self.last_threshold:
            self.activate_alarm()
        else:
            self.deactivate_alarm()

if __name__ == "__main__":
    root = tk.Tk()
    app = UartRepeaterApp(root)
    root.mainloop()