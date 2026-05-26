#include <SoftwareSerial.h>

SoftwareSerial rs485(10, 11); // RX, TX

// Функция расчёта вращающейся контрольной суммы
uint8_t calc_rotating_checksum(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum = (sum << 1) | (sum >> 7);  // циклический сдвиг влево
    sum += data[i];
  }
  return sum;
}

// Формирование и отправка пакета
void sendPacket(uint8_t slaveAddr, const String& cmd, uint8_t dataNum, const String& data = "") {
  char addrStr[3];
  sprintf(addrStr, "%02X", slaveAddr);

  char numStr[3];
  sprintf(numStr, "%02X", dataNum);

  String payload = String(addrStr) + cmd + String(numStr) + data;
  uint8_t chksum = calc_rotating_checksum((const uint8_t*)payload.c_str(), payload.length());
  String packet = ">" + payload + String(chksum, HEX) + "\n";
  rs485.print(packet);
}

// Команды чтения
void readBit(uint8_t slaveAddr, uint8_t dataNum) {
  sendPacket(slaveAddr, "R1", dataNum, "");
}

void readByte(uint8_t slaveAddr, uint8_t dataNum) {
  sendPacket(slaveAddr, "RB", dataNum, "");
}

void readWord(uint8_t slaveAddr, uint8_t dataNum) {
  sendPacket(slaveAddr, "RD", dataNum, "");
}


void readLong(uint8_t slaveAddr, uint8_t dataNum) {
  sendPacket(slaveAddr, "RL", dataNum, "");
}

void readText(uint8_t slaveAddr, uint8_t dataNum) {
  sendPacket(slaveAddr, "RX", dataNum, "");
}

void getStatus(uint8_t slaveAddr) {
  sendPacket(slaveAddr, "GS", 0x00, "");
}

// Команды записи
void writeByte(uint8_t slaveAddr, uint8_t dataNum, uint8_t value) {
  sendPacket(slaveAddr, "WB", dataNum, String(value));
}

void writeWord(uint8_t slaveAddr, uint8_t dataNum, uint16_t value) {
  sendPacket(slaveAddr, "WD", dataNum, String(value));
}


void writeLong(uint8_t slaveAddr, uint8_t dataNum, uint32_t value) {
  sendPacket(slaveAddr, "WL", dataNum, String(value));
}

void writeText(uint8_t slaveAddr, uint8_t dataNum, const String& text) {
  String paddedText = text;
  while (paddedText.length() < 8) {
    paddedText += " ";
  }
  paddedText = paddedText.substring(0, 8);
  sendPacket(slaveAddr, "WX", dataNum, paddedText);
}

void setup() {
  Serial.begin(9600);
  rs485.begin(9600);
}

void loop() {
  // Пример использования команд
  readByte(0x02, 0x01);
  delay(1000);
  writeByte(0x03, 0x01, 0xFF);
  delay(1000);
  getStatus(0x04);
  delay(1000);

  // Чтение ответов от Slave-устройств
  while (rs485.available()) {
    char c = rs485.read();
    Serial.write(c);
  }
}
