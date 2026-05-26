#include <SoftwareSerial.h>

// Настройка последовательного порта для RS-485
// Пины RX и TX для SoftwareSerial (можно заменить на аппаратный Serial при необходимости)
SoftwareSerial rs485(10, 11); // RX, TX

// Функция расчёта вращающейся контрольной суммы
// Алгоритм: циклический сдвиг влево на 1 бит, затем прибавление следующего байта данных
uint8_t calc_rotating_checksum(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum = (sum << 1) | (sum >> 7);  // циклический сдвиг: старший бит переходит в младший
    sum += data[i];                 // прибавляем очередной байт данных
  }
  return sum;
}

// Универсальная функция отправки пакета на указанное устройство
// Параметры:
//   slaveAddr — адрес ведомого устройства (в шестнадцатеричном формате, например, 0x02)
//   cmd — команда (строка из 2 символов, например, "RB", "WD" и т.д.)
//   data — дополнительные данные для команды (по умолчанию пустая строка)
void sendPacket(uint8_t slaveAddr, const String& cmd, const String& data = "") {
  char addrStr[3];
  sprintf(addrStr, "%02X", slaveAddr); // преобразуем адрес в строку из 2 шестнадцатеричных символов с ведущим нулём

  String payload = String(addrStr) + cmd + data; // формируем полезную нагрузку: адрес + команда + данные
  uint8_t chksum = calc_rotating_checksum((const uint8_t*)payload.c_str(), payload.length()); // рассчитываем контрольную сумму
  String packet = ">" + payload + String(chksum, HEX) + "\n"; // формируем полный пакет: стартовый символ + полезная нагрузка + контрольная сумма + стоповый байт
  rs485.print(packet); // отправляем пакет через RS-485
}

// Команда: считать бит (R1)
// Параметр bitNum — номер бита, который нужно считать (от 0 до 7)
void readBit(uint8_t slaveAddr, uint8_t bitNum) {
  sendPacket(slaveAddr, "R1", String(bitNum));
}

// Команда: считать 1 байт (RB)
void readByte(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "RB", "");
}

// Команда: считать 2 байта (RD)
void readWord(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "RD", "");
}

// Команда: считать 3 байта (RT)
void readTriple(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "RT", "");
}

// Команда: считать 4 байта (RL)
void readLong(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "RL", "");
}

// Команда: считать текст (8 байт) (RX)
void readText(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "RX", "");
}

// Команда: получить дату (GD)
void getDate(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "GD", "");
}

// Команда: получить время (GT)
void getTime(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "GT", "");
}

// Команда: получить статус (GS)
void getStatus(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "GS", "");
}

// Команда: записать 1 байт (WB)
void writeByte(uint8_t slaveAddr, uint8_t value) {
  sendPacket(slaveAddr, "WB", String(value));
}

// Команда: записать 2 байта (WD)
void writeWord(uint8_t slaveAddr, uint16_t value) {
  sendPacket(slaveAddr, "WD", String(value));
}

// Команда: записать 3 байта (WT)
void writeTriple(uint8_t slaveAddr, uint32_t value) {
  // Оставляем только младшие 3 байта из 32-битного значения
  uint32_t masked = value & 0xFFFFFF;
  sendPacket(slaveAddr, "WT", String(masked));
}

// Команда: записать 4 байта (WL)
void writeLong(uint8_t slaveAddr, uint32_t value) {
  sendPacket(slaveAddr, "WL", String(value));
}

// Команда: записать текст (8 байт) (WX)
// Если текст короче 8 символов, он дополняется пробелами до 8 символов
// Если длиннее — обрезается до 8 символов
void writeText(uint8_t slaveAddr, const String& text) {
  String paddedText = text;
  while (paddedText.length() < 8) {
    paddedText += " "; // дополняем пробелами, если текст короче 8 символов
  }
  paddedText = paddedText.substring(0, 8); // обрезаем до 8 символов, если длиннее
  sendPacket(slaveAddr, "WX", paddedText);
}

// Команда: записать дату (SD)
// Формат даты — строка, например, "2024-06-15"
void setDate(uint8_t slaveAddr, const String& date) {
  sendPacket(slaveAddr, "SD", date);
}

// Команда: записать время (ST)
// Формат времени — строка, например, "14:30:00"
void setTime(uint8_t slaveAddr, const String& time) {
  sendPacket(slaveAddr, "ST", time);
}

void setup() {
  Serial.begin(9600);   // Инициализация основного последовательного порта для отладки (вывод в Serial Monitor)
  rs485.begin(9600);  // Инициализация последовательного порта для связи по RS-485
  Serial.println("Master device started");
}

void loop() {
  static unsigned long lastSendTime = 0;
  const unsigned long interval = 2000; // Интервал между отправкой команд — 2 секунды

  if (millis() - lastSendTime > interval) {
    lastSendTime = millis();

    // Пример последовательного выполнения команд для демонстрации работы протокола

    // 1. Запрос чтения 1 байта от устройства с адресом 0x02
    Serial.println("Sending: Read Byte from 0x02");
    readByte(0x02);

    delay(500); // Небольшая задержка между командами

    // 2. Запись 1 байта (значение 0xFF) на устройство с адресом 0x03
    Serial.println("Sending: Write Byte 0xFF to 0x03");
    writeByte(0x03, 0xFF);

    delay(500);

    // 3. Запрос статуса от устройства с адресом 0x04
    Serial.println("Sending: Get Status from 0x04");
    getStatus(0x04);

    delay(500);

    // 4. Запись текста "HELLO" на устройство с адресом 0x05
    Serial.println("Sending: Write Text 'HELLO' to 0x05");
    writeText(0x05, "HELLO");

    delay(500);

    // 5. Запрос даты от устройства с адресом 0x06
    Serial.println("Sending: Get Date from 0x06");
    getDate(0x06);
  }

  // Чтение ответов от Slave-устройств (если подключены и отвечают)
  while (rs485.available()) {
    char c = rs485.read();
    Serial.write(c); // Выводим полученный символ в Serial Monitor для отладки
  }
}
