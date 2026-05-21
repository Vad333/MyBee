/*
 * Проект: Улей1 — система управления температурой с передачей данных на сервер
 * Версия: 2024-05-21_16
 * Платформа: Arduino Pro Mini + ESP-01 (Wi‑Fi)
 * Файлы: ULEY.ino (основной модуль), ULEY_FUNC.ino (функции и процедуры)
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
  Serial.println("Проект «Улей1» запущен. Ожидание первого измерения...");
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
