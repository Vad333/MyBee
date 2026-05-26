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

// Отправка ответа на запрос
void sendResponse(const String& status, const String& data);

// Функция для обработки команды чтения бита (R1)
void handleReadBit(uint8_t dataNum) {
  if (dataNum < 16) {
    int bitNum = dataNum % 8;
    int bitValue = (dataBytes[dataNum] >> bitNum) & 0x01;
    sendResponse("OK", String(bitValue));
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды чтения байта (RB)
void handleReadByte(uint8_t dataNum) {
  if (dataNum < 16) {
    sendResponse("OK", String(dataBytes[dataNum]));
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды чтения слова (2 байта, RD)
void handleReadWord(uint8_t dataNum) {
  if (dataNum < 16) {
    sendResponse("OK", String(dataWords[dataNum]));
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды чтения тройки байтов (RT)
void handleReadTriple(uint8_t dataNum) {
  if (dataNum < 16) {
    uint32_t triple = dataLongs[dataNum] & 0xFFFFFF;
    sendResponse("OK", String(triple));
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды чтения длинного значения (4 байта, RL)
void handleReadLong(uint8_t dataNum) {
  if (dataNum < 16) {
    sendResponse("OK", String(dataLongs[dataNum]));
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды чтения текста (8 байт, RX)
void handleReadText(uint8_t dataNum) {
  if (dataNum < 16) {
    sendResponse("OK", dataTexts[dataNum]);
  } else {
    sendResponse("ER", "");
  }
}

// Функция для обработки команды получения статуса (GS)
void handleGetStatus() {
  sendResponse("OK", status);
}
