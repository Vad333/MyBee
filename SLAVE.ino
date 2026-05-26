#include <SoftwareSerial.h>
#include "slave_functions.h"

// Настройка последовательного порта для RS-485
SoftwareSerial rs485(10, 11); // RX, TX

#define MY_DEVICE_ADDR 0x02  // Адрес этого ведомого устройства

void setup() {
  Serial.begin(9600);   // Инициализация основного последовательного порта для отладки
  rs485.begin(9600);  // Инициализация последовательного порта для связи по RS-485
  Serial.println("Slave device started with address: 0x");
  Serial.println(MY_DEVICE_ADDR, HEX);

  // Инициализация данных (можно заменить на реальные датчики/реле)
  initializeData();
}

void loop() {
  static String incomingPacket = ""; // Буфер для накопления входящих данных

  while (rs485.available()) {
    char c = rs485.read();
    incomingPacket += c;

    // Если встретили стоповый байт, обрабатываем пакет
    if (c == '\n') {
      parsePacket(incomingPacket, MY_DEVICE_ADDR);
      incomingPacket = "";  // Очищаем буфер
    }
  }
}
