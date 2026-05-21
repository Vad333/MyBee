#include <OneWire.h>
#include <DallasTemperature.h>

// Пины
#define TEMP_PIN1 2
#define TEMP_PIN2 3
#define RELAY1_PIN 4
#define RELAY2_PIN 5
#define LIGHT_PIN A0
#define LED_PIN 13

// Пороги температуры (°C)
const float T1 = 20.0;  // включение реле 1 при T < T1
const float T2 = 30.0;  // включение реле 2 при T > T2

// Гистерезис
const float HYSTERESIS = 2.0;

// Интервалы времени
const unsigned long MEASUREMENT_INTERVAL = 600000UL;  // 10 минут (в мс)
const unsigned long INTER_MEASUREMENT_DELAY = 10000UL;     // 10 секунд (в мс)

// Объекты
OneWire oneWire1(TEMP_PIN1);
OneWire oneWire2(TEMP_PIN2);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);

// Переменные для хранения времени
unsigned long previousMeasurementTime = 0;

void setup() {
  Serial.begin(9600);
  sensors1.begin();
  sensors2.begin();

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, HIGH);  // реле выключены (активный низкий)
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);     // светодиод выключен

  Serial.println("Система запущена. Ожидание первого измерения...");
}

void loop() {
  unsigned long currentTime = millis();

  // Проверка, пора ли делать измерение (каждые 10 минут)
  if (currentTime - previousMeasurementTime >= MEASUREMENT_INTERVAL) {
    previousMeasurementTime = currentTime;

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

    if (shouldRelay2BeOn) {
      digitalWrite(RELAY2_PIN, LOW);  // включить реле 2 (охлаждение)
      digitalWrite(RELAY1_PIN, HIGH); // принудительно выключить реле 1
      Serial.println("Реле 2: ВКЛ (температура > T2), Реле 1: ВЫКЛ (приоритет реле 2)");
    } else if (shouldRelay2BeOff) {
      digitalWrite(RELAY2_PIN, HIGH); // выключить реле 2

      // Теперь проверяем условия для реле 1 (только если реле 2 выключено)
      if (shouldRelay1BeOn) {
        digitalWrite(RELAY1_PIN, LOW);  // включить реле 1 (обогрев)
        Serial.println("Реле 1: ВКЛ (температура < T1)");
      } else if (shouldRelay1BeOff) {
        digitalWrite(RELAY1_PIN, HIGH); // выключить реле 1
        Serial.println("Реле 1: ВЫКЛ (температура > T1+2)");
      }
    }

    // Выключаем светодиод после измерений
    digitalWrite(LED_PIN, LOW);
  }
}

// Функция для чтения температуры с датчика
float getTemperature(DallasTemperature& sensor) {
  sensor.requestTemperatures();
  return sensor.getTempCByIndex(0);
}
