from machine import UART, Pin
import time

class RS485Master:
    def __init__(self, uart_id=1, tx_pin=1, rx_pin=3, de_re_pin=5, baudrate=9600):
        """
        Инициализация Master-устройства для RS-485.
        :param uart_id: UART1 (аппаратный UART на GPIO1/TX и GPIO3/RX).
        :param tx_pin: пин TX (GPIO1).
        :param rx_pin: пин RX (GPIO3).
        :param de_re_pin: пин для управления DE/RE на MAX485 (GPIO5).
        :param baudrate: скорость передачи.
        """
        # На ESP8266 UART0 используется для USB/REPL, поэтому используем UART1
        self.uart = UART(uart_id, baudrate=baudrate, bits=8, parity=None, stop=1)
        self.de_re = Pin(de_re_pin, Pin.OUT)
        self.de_re.off()  # Изначально в режиме приёма (RE=LOW)

    def calc_rotating_checksum(self, data):
        """
        Расчёт вращающейся контрольной суммы (совместим с Arduino-версией).
        :param data: строка с полезной нагрузкой.
        :return: контрольная сумма в шестнадцатеричном формате (2 символа).
        """
        sum_val = 0
        for byte in data.encode('ascii'):
            sum_val = (sum_val << 1) | (sum_val >> 7)
            sum_val += byte
            sum_val &= 0xFF  # Ограничиваем значение до 1 байта
        return f"{sum_val:02X}"

    def send_packet(self, packet):
        """
        Отправка пакета через RS-485.
        :param packet: строка пакета для отправки.
        """
        # Переключаем в режим передачи (DE=HIGH, RE=LOW)
        self.de_re.on()
        time.sleep_ms(1)  # Небольшая задержка для стабилизации
        self.uart.write(packet.encode('ascii'))
        time.sleep_ms(10)  # Ждём, пока данные уйдут
        # Возвращаемся в режим приёма (DE=LOW, RE=HIGH)
        self.de_re.off()

    def send_command(self, device_addr, command, data_num, data_value=""):
        """
        Формирование и отправка команды ведомому устройству.
        :param device_addr: адрес устройства (в шестнадцатеричном формате, например, '02').
        :param command: код команды (например, 'WB', 'RB').
        :param data_num: номер ячейки данных (в шестнадцатеричном формате).
        :param data_value: опциональные данные (для команд записи).
        :return: ответ от ведомого устройства или None при ошибке.
        """
        payload = f"{device_addr}{command}{data_num}{data_value}"
        checksum = self.calc_rotating_checksum(payload)
        packet = f">{payload}{checksum}\n"
        print(f"Отправка пакета: {packet.strip()}")
        self.send_packet(packet)

        # Ждём ответ
        start_time = time.ticks_ms()
        while time.ticks_diff(time.ticks_ms(), start_time) < 1000:  # Таймаут 1 секунда
            if self.uart.any():
                response = self.uart.readline()
                if response:
                    response = response.decode('ascii').strip()
                    print(f"Получен ответ: {response}")
                    return response
        print("Таймаут ожидания ответа")
        return None

    def read_byte(self, device_addr, data_num):
        """
        Чтение байта из указанного номера ячейки.
        :param device_addr: адрес устройства.
        :param data_num: номер ячейки (в шестнадцатеричном формате).
        :return: значение байта или None при ошибке.
        """
        response = self.send_command(device_addr, "RB", data_num)
        if response and response.startswith("<"):
            status = response[2:4]
            if status == "OK":
                data_start = 4
                data_end = -2
                data = response[data_start:data_end]
                return int(data)
        return None

    def write_byte(self, device_addr, data_num, value):
        """
        Запись байта в указанный номер ячейки.
        :param device_addr: адрес устройства.
        :param data_num: номер ячейки (в шестнадцатеричном формате).
        :param value: значение для записи (целое число от 0 до 255).
        :return: True при успехе, False при ошибке.
        """
        value_str = str(value)
        response = self.send_command(device_addr, "WB", data_num, value_str)
        if response and response.startswith("<"):
            status = response[2:4]
            return status == "OK"
        return False

# Пример использования
if __name__ == "__main__":
    # Инициализация Master
    master = RS485Master(uart_id=1, tx_pin=1, rx_pin=3, de_re_pin=5, baudrate=9600)

    try:
        # Пример: запись байта со значением 255 в ячейку №1 устройства с адресом 0x02
        success = master.write_byte("02", "01", 255)
        if success:
            print("Запись прошла успешно")
        else:
            print("Ошибка при записи")

        # Пример: чтение байта из ячейки №1 устройства с адресом 0x02
        value = master.read_byte("02", "01")
        if value is not None:
            print(f"Прочитанное значение: {value}")
        else:
            print("Ошибка при чтении")

    except Exception as e:
        print(f"Ошибка: {e}")
