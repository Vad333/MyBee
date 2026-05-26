import serial
import time

class RS485Master:
    def __init__(self, port, baudrate=9600, timeout=1):
        """
        Инициализация соединения с последовательным портом.
        :param port: имя порта (например, 'COM3' в Windows или '/dev/ttyUSB0' в Linux).
        :param baudrate: скорость передачи данных.
        :param timeout: таймаут ожидания ответа.
        """
        self.ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=timeout
        )
        time.sleep(2)  # Даём время на инициализацию порта

    def calc_rotating_checksum(self, data):
        """
        Расчёт вращающейся контрольной суммы.
        :param data: строка с полезной нагрузкой (без стартового символа и контрольной суммы).
        :return: контрольная сумма в шестнадцатеричном формате (2 символа).
        """
        sum_val = 0
        for byte in data.encode('ascii'):
            sum_val = (sum_val << 1) | (sum_val >> 7)
            sum_val += byte
            sum_val &= 0xFF  # Ограничиваем значение до 1 байта
        return f"{sum_val:02X}"

    def send_command(self, device_addr, command, data_num, data_value=""):
        """
        Отправка команды ведомому устройству.
        :param device_addr: адрес устройства (в шестнадцатеричном формате, например, '02').
        :param command: код команды (например, 'WB', 'RB').
        :param data_num: номер ячейки данных (в шестнадцатеричном формате, например, '01').
        :param data_value: опциональные данные (для команд записи).
        :return: ответ от ведомого устройства или None при ошибке.
        """
        # Формируем полезную нагрузку: адрес + команда + номер ячейки + данные
        payload = f"{device_addr}{command}{data_num}{data_value}"
        # Рассчитываем контрольную сумму для полезной нагрузки
        checksum = self.calc_rotating_checksum(payload)
        # Формируем полный пакет: стартовый символ + полезная нагрузка + контрольная сумма + стоповый символ
        packet = f">{payload}{checksum}\n"
        print(f"Отправка пакета: {packet.strip()}")

        # Отправляем пакет
        self.ser.write(packet.encode('ascii'))

        # Ждём ответ
        response = self.ser.readline().decode('ascii').strip()
        if response:
            print(f"Получен ответ: {response}")
            return response
        else:
            print("Ответ не получен")
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
            # Парсим ответ: <AASS[DATA][CHK]
            status = response[2:4]
            if status == "OK":
                # Извлекаем данные (между статусом и контрольной суммой)
                data_start = 4
                data_end = -2  # Последние 2 символа — контрольная сумма
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

    def close(self):
        """Закрытие последовательного порта."""
        if self.ser.is_open:
            self.ser.close()

# Пример использования
if __name__ == "__main__":
    # Инициализация Master (замените 'COM3' на ваш порт)
    master = RS485Master(port='COM3', baudrate=9600)

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

    finally:
        master.close()
