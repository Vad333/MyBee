#ifndef SLAVE_FUNCTIONS_H
#define SLAVE_FUNCTIONS_H

#include <SoftwareSerial.h>
#include <Arduino.h>

// Объявление внешних переменных для хранения данных
extern uint8_t currentByte;
extern uint16_t currentWord;
extern uint32_t currentLong;
extern String currentText;
extern String currentDate;
extern String currentTime;
extern String status;

// Функция расчёта вращающейся контрольной суммы
// На каждом шаге выполняется циклический сдвиг влево на 1 бит,
// затем к результату прибавляется очередной байт данных
uint8_t calc_rotating_checksum(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum = (sum << 1) | (sum >> 7);  // Циклический сдвиг: старший бит переходит в младший
    sum += data[i];                 // Прибавляем очередной байт данных
  }
  return sum;
}

// Формирование и отправка ответа ведомому устройству
// Параметры:
//   status — строка статуса (например, "OK" или "ER")
//   data — данные, которые нужно передать в ответе (по умолчанию пустая строка)
void sendResponse(const String& status, const String& data = "") {
  char addrStr[3];
  sprintf(addrStr, "%02X", MY_DEVICE_ADDR); // Преобразуем адрес в строку из 2 шестнадцатеричных символов

  String payload = String(addrStr) + status + data; // Формируем полезную нагрузку: адрес + статус + данные
  uint8_t chksum = calc_rotating_checksum((const uint8_t*)payload.c_str(), payload.length()); // Рассчитываем контрольную сумму
  String response = "<" + payload + String(chksum, HEX) + "\n"; // Формируем полный ответ
  rs485.print(response); // Отправляем ответ через RS-485
}

// Инициализация тестовых данных при запуске устройства
void initializeData() {
  currentByte = 0x55;
  currentWord = 0xABCD;
  currentLong = 0x12345678;
  currentText = "ABCDEFGH";
  currentDate = "2024-06-15";
  currentTime = "14:30:00";
  status = "OK01";
}

// Парсинг входящего пакета от Master
// Параметр packet — строка с полученным пакетом
void parsePacket(String packet, uint8_t myAddr) {
  if (packet.length() < 6) return;  // Слишком короткий пакет — игнорируем
  if (packet[0] != '>') return;     // Неверный стартовый символ — игнорируем

  // Извлекаем адрес устройства из пакета
  char addrStr[3] = {packet[1], packet[2], '\0'};
  uint8_t addr = (uint8_t)strtol(addrStr, NULL, 16);
  if (addr != myAddr) return; // Пакет не для этого устройства — игнорируем

  // Выделяем полезную нагрузку (без стартового символа '>' и стопового '\n')
  int payloadLen = packet.length() - 4; // -1(>) -1(\n) -2(CHK)
  String payload = packet.substring(1, 1 + payloadLen);

  // Проверяем контрольную сумму
  char chkStr[3] = {packet[packet.length() - 3], packet[packet.length() - 2], '\0'};
  uint8_t receivedChksum = (uint8_t)strtol(chkStr, NULL, 16); // Полученная контрольная сумма из пакета
  uint8_t calculatedChksum = calc_rotating_checksum((const uint8_t*)payload.c_str(), payload.length()); // Расчёт контрольной суммы для полученной полезной нагрузки

  if (receivedChksum != calculatedChksum) {
    sendResponse("ER", ""); // Если контрольная сумма не совпадает, отправляем ошибку
    return;
  }

  // Обрабатываем команду
  String cmd = packet.substring(3, 5);
  if (cmd == "R1") {
    // Команда: считать бит (R1)
    int bitNum = packet.substring(5).toInt();
    if (bitNum >= 0 && bitNum <= 7) {
      int bitValue = (currentByte >> bitNum) & 0x01; // Извлекаем нужный бит из currentByte
      sendResponse("OK", String(bitValue));
    } else {
      sendResponse("ER", "");
    }
  } else if (cmd == "RB") {
    // Команда: считать 1 байт (RB)
    sendResponse("OK", String(currentByte));
  } else if (cmd == "RD") {
    // Команда: считать 2 байта (RD)
    sendResponse("OK", String(currentWord));
  } else if (cmd == "RT") {
    // Команда: считать 3 байта (RT)
    uint32_t triple = currentLong & 0xFFFFFF; // Оставляем младшие 3 байта из currentLong
    sendResponse("OK", String(triple));
  } else if (cmd == "RL") {
    // Команда: считать 4 байта (RL)
    sendResponse("OK", String(currentLong));
  } else if (cmd == "RX") {
    // Команда: считать текст (8 байт) (RX)
    sendResponse("OK", currentText);
  } else if (cmd == "GD") {
    // Команда: получить дату (GD)
    sendResponse("OK", currentDate);
  } else if (cmd == "GT") {
    // Команда: получить время (GT)
    sendResponse("OK", currentTime);
  } else if (cmd == "GS") {
    // Команда: получить статус (GS)
    sendResponse("OK", status);
  } else if (cmd == "WB") {
    // Команда: записать 1 байт (WB)
    currentByte = packet.substring(5).toInt(); // Обновляем значение currentByte
    sendResponse("OK", "");
  } else if (cmd == "WD") {
    // Команда: записать 2 байта (WD)
    currentWord = packet.substring(5).toUInt(); // Обновляем значение currentWord
    sendResponse("OK", "");
  } else if (cmd == "WT") {
    // Команда: записать 3 байта (WT)
    uint32_t value = packet.substring(5).toULong();
    currentLong = (currentLong & 0xFF000000) | (value & 0x00FFFFFF); // Обновляем младшие 3 байта currentLong
    sendResponse("OK", "");
  } else if (cmd == "WL") {
    // Команда: записать 4 байта (WL)
    currentLong = packet.substring(5).toULong(); // Обновляем значение currentLong
    sendResponse("OK", "");
  } else if (cmd == "WX") {
    // Команда: записать текст (8 байт) (WX)
    String newText = packet.substring(5, 13); // Извлекаем 8 символов текста
    currentText = newText; // Обновляем значение currentText
    sendResponse("OK", "");
  } else if (cmd == "SD") {
    // Команда: записать дату (SD)
    currentDate = packet.substring(5); // Обновляем значение currentDate
    sendResponse("OK", "");
  } else if (cmd == "ST") {
    // Команда: записать время (ST)
    currentTime = packet.substring(5); // Обновляем значение currentTime
    sendResponse("OK", "");
  } else {
    // Неизвестная команда — отправляем ошибку
    sendResponse("ER", "");
  }
}

#endif // SLAVE_FUNCTIONS_H
