#include <SoftwareSerial.h>
#include "slave_functions.h"

// Настройка последовательного порта для RS-485
SoftwareSerial rs485(10, 11); // RX, TX

// Адрес этого ведомого устройства (в шестнадцатеричном формате)
uint8_t MY_DEVICE_ADDR = 0x02;

// Массивы для хранения данных по номерам
uint8_t dataBytes[16];
uint16_t dataWords[16];
uint32_t dataLongs[16];
String dataTexts[16];
String status;

void setup() {
  Serial.begin(9600);   // Инициализация основного последовательного порта для отладки
  rs485.begin(9600);  // Инициализация последовательного порта для связи по RS-485
  Serial.println("Slave device started with address: 0x");
  Serial.println(MY_DEVICE_ADDR, HEX);

  // Инициализация данных
  initializeData();
}

void loop() {
  static String incomingPacket = ""; // Буфер для накопления входящих данных

  while (rs485.available()) {
    char c = rs485.read();
    incomingPacket += c;

    // Если встретили стоповый байт, обрабатываем пакет
    if (c == '\n') {
      parsePacket(incomingPacket);
      incomingPacket = "";  // Очищаем буфер
    }
  }
}
