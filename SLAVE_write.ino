#include <Arduino.h>
#include <SoftwareSerial.h>

// Внешние объявления — соответствуют переменным из основного файла
extern SoftwareSerial rs485;
extern uint8_t MY_DEVICE_ADDR;
extern uint8_t dataBytes[16];
extern uint16_t dataWords[16];
extern uint32_t dataLongs[16];
extern String dataTexts[16];
extern String status;

// Функция расчёта вращающейся контрольной суммы
uint8_t calc_rotating_checksum(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum = (sum << 1) | (sum >> 7);
    sum += data[i];
  }
  return sum;
}

// Формирование и отправка ответа
void sendResponse(const String& status, const String& data) {
  char addrStr[3];
  sprintf(addrStr, "%02X", MY_DEVICE_ADDR);

  String payload = String(addrStr) + status + data;
  uint8_t chksum = calc_rotating_checksum((const uint8_t*)payload.c_str(), payload.length());
  String response = "<" + payload + String(chksum, HEX) + "\n";
  rs485.print(response);
}

// Инициализация тестовых данных при запуске устройства
void initializeData() {
  for (int i = 0; i < 16; i++) {
    dataBytes[i] = 0x00;
    dataWords[i] = 0x0000;
    dataLongs[i] = 0x00000000;
    dataTexts[i] = "        ";
  }
  status = "OK01";
}

// Функция для обработки команды записи байта (WB)
void handleWriteByte(uint8_t dataNum, String valueStr) {
  if (dataNum < 16) {
    dataBytes[dataNum] = valueStr.toInt();
    sendResponse("OK", "");
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды записи слова (2 байта, WD)
void handleWriteWord(uint8_t dataNum, String valueStr) {
  if (dataNum < 16) {
    dataWords[dataNum] = valueStr.toUInt();
    sendResponse("OK", "");
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды записи длинного значения (4 байта, WL)
void handleWriteLong(uint8_t dataNum, String valueStr) {
  if (dataNum < 16) {
    dataLongs[dataNum] = valueStr.toULong();
    sendResponse("OK", "");
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды записи текста (8 байт, WX)
void handleWriteText(uint8_t dataNum, String newText) {
  if (dataNum < 16) {
    if (newText.length() > 8) {
      newText = newText.substring(0, 8);
    } else {
      while (newText.length() < 8) {
        newText += " ";
      }
    }
    dataTexts[dataNum] = newText;
    sendResponse("OK", "");
  } else {
    sendResponse("ER", "");
  }
}

// Парсинг входящего пакета от Master
void parsePacket(String packet) {
  if (packet.length() < 8) return;
  if (packet[0] != '>') return;

  // Извлекаем адрес устройства из пакета
  char addrStr[3] = {packet[1], packet[2], '\0'};
  uint8_t addr = (uint8_t)strtol(addrStr, NULL, 16);
  if (addr != MY_DEVICE_ADDR) return;

  // Выделяем полезную нагрузку
  int payloadLen = packet.length() - 4;
  String payload = packet.substring(1, 1 + payloadLen);

  // Проверяем контрольную сумму
  char chkStr[3] = {packet[packet.length() - 3], packet[packet.length() - 2], '\0'};
  uint8_t receivedChksum = (uint8_t)strtol(chkStr, NULL, 16);
  uint8_t calculatedChksum = calc_rotating_checksum((const uint8_t*)payload.c_str(), payload.length());

  if (receivedChksum != calculatedChksum) {
    sendResponse("ER", "");
    return;
  }

  // Обрабатываем команду
  String cmd = packet.substring(3, 5);
  char numStr[3] = {packet[5], packet[6], '\0'};
  uint8_t dataNum = (uint8_t)strtol(numStr, NULL, 16);

  if (cmd == "WB") {
    handleWriteByte(dataNum, packet.substring(7));
  } else if (cmd == "WD") {
    handleWriteWord(dataNum, packet.substring(7));
  } else if (cmd == "WL") {
    handleWriteLong(dataNum, packet.substring(7));
  } else if (cmd == "WX") {
    handleWriteText(dataNum, packet.substring(7));
  } else if (cmd == "GS") {
    handleGetStatus();
  } else if (cmd == "R1") {
    handleReadBit(dataNum);
  } else if (cmd == "RB") {
    handleReadByte(dataNum);
  } else if (cmd == "RD") {
    handleReadWord(dataNum);
  } else if (cmd == "RL") {
    handleReadLong(dataNum);
  } else if (cmd == "RX") {
    handleReadText(dataNum);
  } else {
    sendResponse("ER", ""); // Неизвестная команда
  }
}
