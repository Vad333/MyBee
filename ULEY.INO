/*
 * Проект: Система управления температурой с передачей данных на сервер
 * Версия: 2024-05-21_16
 * Платформа: Arduino Pro Mini + ESP-01 (Wi‑Fi)
 * Компоненты: 2×DS18B20, 2×реле, фоторезистор, кнопка
 * Функции:
 *   - Измерение температуры каждые 10 минут (двойное измерение с усреднением)
 *   - Принудительный опрос температуры по нажатию кнопки
 *   - Управление реле с гистерезисом и приоритетом реле 2
 *   - Передача данных на сервер: каждый час и при переключении реле
 *   - Индикация процесса измерения встроенным светодиодом
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>

// Пины
#define TEMP_PIN1 2
#define TEMP_PIN2 3
#define RELAY1_PIN 4
#define RELAY2_PIN 5
#define LIGHT_PIN A0
#define LED_PIN 13
#define ESP_TX_PIN 6
#define ESP_RX_PIN 7
#define BUTTON_PIN 8

// Настройки Wi‑Fi и сервера
const char* SSID = "your_wifi_ssid";
const char* PASSWORD = "your_wifi_password";
const char* SERVER_URL = "http://yourserver.com/api/data";

// Пороги температуры (°C)
const float T1 = 20.0;
const float T2 = 30.0;

// Гистерезис
const float HYSTERESIS = 2.0;

// Интервалы времени
const unsigned long MEASUREMENT_INTERVAL = 600000UL;  // 10 минут
const unsigned long INTER_MEASUREMENT_DELAY = 10000UL;     // 10 секунд
const unsigned long HOURLY_SEND_INTERVAL = 3600000UL;       // 1 час

// Объекты
OneWire oneWire1(TEMP_PIN1);
OneWire oneWire2(TEMP_PIN2);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN);  // RX, TX

// Переменные для хранения времени
unsigned long previousMeasurementTime = 0;
unsigned long lastHourlySendTime = 0;

// Переменные состояния реле
bool relay1State = false;
bool relay2State = false;

// Переменная для отслеживания состояния кнопки
bool buttonPressed = false;

void setup() {
  Serial.begin(9600);
  espSerial.begin(115200);  // скорость для ESP‑01
  sensors1.begin();
  sensors2.begin();

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // внутренний подтягивающий резистор

  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);

  connectToWiFi();
  Serial.println("Система запущена. Ожидание первого измерения...");
}

void loop() {
  unsigned long currentTime = millis();

  // Проверка нажатия кнопки
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // антидребезг
    if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
      buttonPressed = true;
      Serial.println("Кнопка нажата — принудительный опрос температуры");
      performMeasurementAndControl();
    }
  } else {
    buttonPressed = false;  // кнопка отпущена
  }

  // Проверка, пора ли делать измерение (каждые 10 минут)
  if (currentTime - previousMeasurementTime >= MEASUREMENT_INTERVAL) {
    previousMeasurementTime = currentTime;
    performMeasurementAndControl();
  }

  // Проверка, пора ли отправлять данные (каждый час)
  if (currentTime - lastHourlySendTime >= HOURLY_SEND_INTERVAL) {
    lastHourlySendTime = currentTime;
    sendDataToServer();
  }
}

void performMeasurementAndControl() {
  // Зажигаем светодиод на время измерений
  digitalWrite(LED_PIN, HIGH);

  // Измеряем температуру дважды с интервалом 10 сек и вычисляем среднее
  float temp1_1 = getTemperature(sensors1);
  delay(INTER_MEASUREMENT_DELAY);
  float temp1_2 = getTemperature(sensors1);
  float avgTemp1 = (temp1_1 + temp1_2) / 2.0;

  float temp2_1 = getTemperature(sensors2);
  delay(INTER_MEASUREMENT_DELAY);
  float temp2_2 = getTemperature(sensors2);
  float avgTemp2 = (temp2_1 + temp2_2) / 2.0;

  // Читаем освещённость
  int lightLevel = analogRead(LIGHT_PIN);

  Serial.print("Средняя температура 1: "); Serial.print(avgTemp1); Serial.println(" C");
  Serial.print("Средняя температура 2: "); Serial.print(avgTemp2); Serial.println(" C");
  Serial.print("Освещённость: "); Serial.print(lightLevel); Serial.println();

  // Логика управления реле с приоритетом реле 2
  bool shouldRelay2BeOn = (avgTemp2 > T2);
  bool shouldRelay2BeOff = (avgTemp2 < T2 - HYSTERESIS);
  bool shouldRelay1BeOn = (avgTemp1 < T1);
  bool shouldRelay1BeOff = (avgTemp1 > T1 + HYSTERESIS);


  bool newRelay1State = relay1State;
  bool newRelay2State = relay2State;

  if (shouldRelay2BeOn) {
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY1_PIN, HIGH);
    newRelay2State = true;
    newRelay1State = false;
    Serial.println("Реле 2: ВКЛ (температура > T2), Реле 1: ВЫКЛ (приоритет реле 2)");
  } else if (shouldRelay2BeOff) {
    digitalWrite(RELAY2_PIN, HIGH);
    newRelay2State = false;

    if (shouldRelay1BeOn) {
      digitalWrite(RELAY1_PIN, LOW);
      newRelay1State = true;
      Serial.println("Реле 1: ВКЛ (температура < T1)");
    } else if (shouldRelay1BeOff) {
      digitalWrite(RELAY1_PIN, HIGH);
      newRelay1State = false;
      Serial.println("Реле 1: ВЫКЛ (температура > T1+2)");
    }
  }

  // Проверяем, изменилось ли состояние реле
  if (newRelay1State != relay1State || newRelay2State != relay2State) {
    relay1State = newRelay1State;
    relay2State = newRelay2State;
    // Отправляем данные при переключении реле
    sendDataToServer();
  }

  // Выключаем светодиод после измерений
  digitalWrite(LED_PIN, LOW);
}

void sendDataToServer() {
  String data = "T1=" + String(T1) +
              "&T2=" + String(T2) +
              "&Relay1=" + String(relay1State ? 1 : 0) +
              "&Relay2=" + String(relay2State ? 1 : 0);

  // Здесь должна быть реализация отправки HTTP POST запроса на сервер
  // Пример: AT+CIPSEND=... для ESP‑01
  Serial.println("Отправка данных на сервер: " + data);
  // В реальной реализации здесь будет код для отправки данных через ESP‑01
}

void connectToWiFi() {
  espSerial.println("AT");
  delay(1000);
  if (espSerial.find("OK")) {
    Serial.println("ESP‑01 готов");
  }
  espSerial.println("AT+CWMODE=1");
  delay(1000);
  espSerial.println("AT+CWJAP="\"" + String(SSID) + "\",\"" + String(PASSWORD) + "\"");
  delay(5000);
}
