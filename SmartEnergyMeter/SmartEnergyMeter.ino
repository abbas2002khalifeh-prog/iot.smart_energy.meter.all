#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ======================================================
// I2C LCD
// ======================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======================================================
// WIFI CONFIG
// ======================================================

const char* WIFI_SSID     = "your wifi ssid 2.4ghz";
const char* WIFI_PASSWORD = "your wifi pass";

// ======================================================
// MQTT CONFIG
// ======================================================

const char* MQTT_BROKER = "your broker ip";
const int MQTT_PORT = 1883;

const char* MQTT_CLIENT = "esp32_energy_meter";

const char* TOPIC_LIVE   = "energy/meter/live";
const char* TOPIC_STATUS = "energy/meter/status";

// ======================================================
// SENSOR PINS
// ======================================================

const int PIN_CURRENT = 34;
const int PIN_VOLTAGE = 35;

// ======================================================
// CALIBRATION
// ======================================================

float CURRENT_CALIBRATION = 60.0;
float VOLTAGE_CALIBRATION = 654.0;
float VOLTAGE_OFFSET = 0.0;

// ======================================================
// SETTINGS
// ======================================================

const float ELECTRICITY_RATE = 0.11;
const float POWER_ALERT_THRESHOLD = 2000.0;

const float CURRENT_NOISE_FLOOR = 0.03f;

const int SAMPLES = 500;

const unsigned long UPDATE_INTERVAL = 2000;

// ======================================================
// GLOBALS
// ======================================================

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

float g_vrms  = 0.0;
float g_irms  = 0.0;

float g_power = 0.0;
float g_energy = 0.0;
float g_cost = 0.0;

bool g_alert = false;

unsigned long lastUpdate = 0;
unsigned long lastReconnect = 0;

// ======================================================
// RMS MEASUREMENT
// ======================================================

float measureRMS(int pin) {

  int samples[SAMPLES];

  long sum = 0;

  for (int i = 0; i < SAMPLES; i++) {

    samples[i] = analogRead(pin);

    sum += samples[i];

    delayMicroseconds(10);
  }

  float mean = (float)sum / SAMPLES;

  float sumSq = 0;

  for (int i = 0; i < SAMPLES; i++) {

    float v = samples[i] - mean;

    sumSq += v * v;
  }

  float rms = sqrt(sumSq / SAMPLES);

  return rms * (3.3 / 4096.0);
}

// ======================================================
// CURRENT SENSOR
// ======================================================

float readCurrent() {

  float volts = measureRMS(PIN_CURRENT);

  float current = volts * CURRENT_CALIBRATION;

  if (current < CURRENT_NOISE_FLOOR) {

    current = 0.0;
  }

  return current;
}

// ======================================================
// VOLTAGE SENSOR
// ======================================================

float readVoltage() {

  float volts = measureRMS(PIN_VOLTAGE);

  float voltage =
    (volts * VOLTAGE_CALIBRATION)
    + VOLTAGE_OFFSET;

  if (voltage < 10.0) {

    voltage = 0.0;
  }

  return voltage;
}

// ======================================================
// WIFI CONNECT
// ======================================================

void connectWiFi() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.mode(WIFI_STA);

  WiFi.setAutoReconnect(true);

  WiFi.persistent(true);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("[WiFi] Connected");

  Serial.print("[WiFi IP] ");
  Serial.println(WiFi.localIP());

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");

  delay(1000);
}

// ======================================================
// MQTT CONNECT
// ======================================================

bool connectMQTT() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Connecting MQTT");

  Serial.println("[MQTT] Connecting...");

  bool connected = mqtt.connect(
    MQTT_CLIENT,
    TOPIC_STATUS,
    1,
    true,
    "offline"
  );

  if (connected) {

    mqtt.publish(
      TOPIC_STATUS,
      "online",
      true
    );

    Serial.println("[MQTT] Connected");

    lcd.setCursor(0, 1);
    lcd.print("MQTT Connected");

  } else {

    Serial.println("[MQTT] Failed");

    lcd.setCursor(0, 1);
    lcd.print("MQTT Failed");
  }

  delay(1000);

  return connected;
}

// ======================================================
// MQTT PUBLISH
// ======================================================

void publishData() {

  StaticJsonDocument<256> doc;

  doc["ts"] = millis();

  doc["v_rms"] = g_vrms;
  doc["i_rms"] = g_irms;

  doc["power_w"] = g_power;

  doc["energy_kwh"] = g_energy;

  doc["cost_usd"] = g_cost;

  doc["alert"] = g_alert;

  char payload[256];

  serializeJson(doc, payload);

  mqtt.publish(TOPIC_LIVE, payload);

  Serial.println("[MQTT] Data Published");
}

// ======================================================
// LCD UPDATE
// ======================================================

void updateLCD() {

  static bool screen = false;

  // ALERT SCREEN
  if (g_alert) {

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("!!! OVERLOAD");

    lcd.setCursor(0, 1);
    lcd.print("P:");
    lcd.print((int)g_power);
    lcd.print("W");

    return;
  }

  // SCREEN 1
  if (!screen) {

    lcd.clear();

    char line1[17];

    snprintf(
      line1,
      sizeof(line1),
      "V:%3.0f I:%2.1f",
      g_vrms,
      g_irms
    );

    lcd.setCursor(0, 0);
    lcd.print(line1);

    char line2[17];

    snprintf(
      line2,
      sizeof(line2),
      "P:%4.0fW",
      g_power
    );

    lcd.setCursor(0, 1);
    lcd.print(line2);
  }

  // SCREEN 2
  else {

    lcd.clear();

    char line1[17];

    snprintf(
      line1,
      sizeof(line1),
      "E:%1.4fkWh",
      g_energy
    );

    lcd.setCursor(0, 0);
    lcd.print(line1);

    char line2[17];

    snprintf(
      line2,
      sizeof(line2),
      "C:$%1.4f",
      g_cost
    );

    lcd.setCursor(0, 1);
    lcd.print(line2);
  }

  screen = !screen;
}

// ======================================================
// SERIAL OUTPUT
// ======================================================

void printSerial() {

  Serial.println("==============");

  Serial.print("Voltage : ");
  Serial.print(g_vrms, 2);
  Serial.println(" V");

  Serial.print("Current : ");
  Serial.print(g_irms, 2);
  Serial.println(" A");

  Serial.print("Power   : ");
  Serial.print(g_power, 2);
  Serial.println(" W");

  Serial.print("Energy  : ");
  Serial.print(g_energy, 5);
  Serial.println(" kWh");

  Serial.print("Cost    : ");
  Serial.print(g_cost, 4);
  Serial.println(" $");

  Serial.print("WiFi Status: ");
  Serial.println(WiFi.status());

  Serial.print("MQTT Connected: ");
  Serial.println(mqtt.connected());
}

// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  // LCD
  Wire.begin(21, 22);

  lcd.init();

  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Smart Energy");

  lcd.setCursor(0, 1);
  lcd.print("System Boot");

  delay(2000);

  // ADC
  analogReadResolution(12);

  analogSetAttenuation(ADC_11db);

  pinMode(PIN_CURRENT, INPUT);
  pinMode(PIN_VOLTAGE, INPUT);

  // MQTT SETTINGS
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);

  mqtt.setKeepAlive(60);

  mqtt.setSocketTimeout(60);

  mqtt.setBufferSize(512);

  // WIFI
  connectWiFi();

  // MQTT
  connectMQTT();

  lastUpdate = millis();
}

// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

  unsigned long now = millis();

  // ==================================================
  // WIFI RECONNECT
  // ==================================================

  if (WiFi.status() != WL_CONNECTED &&
      now - lastReconnect > 10000) {

    Serial.println("[WiFi] Reconnecting...");

    WiFi.reconnect();

    lastReconnect = now;
  }

  // ==================================================
  // MQTT RECONNECT
  // ==================================================

  if (!mqtt.connected() &&
      WiFi.status() == WL_CONNECTED &&
      now - lastReconnect > 5000) {

    Serial.println("[MQTT] Reconnecting...");

    connectMQTT();

    lastReconnect = now;
  }

  mqtt.loop();

  // ==================================================
  // SENSOR UPDATE
  // ==================================================

  if (now - lastUpdate >= UPDATE_INTERVAL) {

    // READ SENSORS
    g_vrms = readVoltage();

    g_irms = readCurrent();

    // POWER
    if (g_irms < 0.03f) {

      g_irms = 0.0;

      g_power = 0.0;

    } else {

      g_power = g_vrms * g_irms;
    }

    // ENERGY
    if (g_power > 1.0f) {

      g_energy +=
        (g_power / 1000.0f)
        / 3600.0f;
    }

    // COST
    if (g_energy < 0.0001f) {

      g_cost = 0.0;

    } else {

      g_cost =
        g_energy * ELECTRICITY_RATE;
    }

    // ALERT
    g_alert =
      (g_power > POWER_ALERT_THRESHOLD);

    // OUTPUTS
    printSerial();

    updateLCD();

    if (mqtt.connected()) {

      publishData();
    }

    lastUpdate = now;
  }
}