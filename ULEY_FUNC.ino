// Функция для чтения температуры с датчика
float getTemperature(DallasTemperature& sensor) {
  sensor.requestTemperatures();
  return sensor.getTempCByIndex(0);
}

// Функция выполнения измерения и управления реле
void performMeasurementAndControl() {
  // Зажигаем светодиод на время измерений
  digitalWrite(LED_PIN, HIGH);

  // Измеряем температуру дважды с интервалом 10 сек и вычисляем среднее
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

  // Логика управления реле с приоритетом реле 2
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

// Функция отправки данных на сервер
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

// Функция подключения к Wi‑Fi
void connectToWiFi() {
  espSerial.println("AT");
  delay(1000);
  if (espSerial.find("OK")) {
    Serial.println("ESP‑01 готов");
  }
  espSerial.println("AT+CWMODE=1");
  delay(1000);
  espSerial.println("AT+CWJAP=\"" + String(SSID) + "\",\"" + String(PASSWORD) + "\"");
  delay(5000);
}
