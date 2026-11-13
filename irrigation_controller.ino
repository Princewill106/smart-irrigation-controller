/*
  Smart Irrigation Controller - ESP32
  Sensors: 3x capacitive soil moisture (GPIO 32, 33, 34)
  Relay:   GPIO 26 -> 12V solenoid valve (fused)
  Override switch: GPIO 27 (3-position: AUTO / OFF / ON via GPIO 27 + 25)
  Logs JSON to WEBHOOK_URL every cycle.
*/
#include <WiFi.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "your-wifi";
const char* WIFI_PASS = "your-password";
const char* WEBHOOK_URL = "https://your-dashboard.example/ingest";

const int SENSOR_PINS[3] = {32, 33, 34};
const int RELAY_PIN = 26;
const int OVERRIDE_OFF_PIN = 27;   // pulled LOW = force OFF
const int OVERRIDE_ON_PIN = 25;    // pulled LOW = force ON
const int DRY_RAW = 3200;          // calibrate: reading in dry air
const int WET_RAW = 1300;          // calibrate: reading in water
const int MAX_RUN_MIN = 20;
const float MIN_PER_POINT = 0.8;
const int TARGET_OFFSET = 12;
const float RAIN_SKIP_MM = 5.0;
const unsigned long CYCLE_MS = 10UL * 60UL * 1000UL;

int cropDay = 0;                   // set from your dashboard or EEPROM
float forecastMm = 0;              // updated by your dashboard/webhook response

int threshold(int day) {
  if (day <= 4) return 45;         // seedling
  if (day <= 9) return 38;         // vegetative
  return 42;                       // flowering
}

float readPercent(int pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(pin); delay(20); }
  float raw = sum / 10.0;
  float pct = (DRY_RAW - raw) * 100.0 / (DRY_RAW - WET_RAW);
  return constrain(pct, 0, 100);
}

float median3(float a, float b, float c) {
  return max(min(a, b), min(max(a, b), c));
}

int decide(float m, int day, float forecast, String &reason) {
  if (digitalRead(OVERRIDE_OFF_PIN) == LOW) { reason = "manual override OFF"; return 0; }
  if (digitalRead(OVERRIDE_ON_PIN) == LOW)  { reason = "manual override ON";  return MAX_RUN_MIN; }
  int th = threshold(day);
  if (m >= th) { reason = "moist enough"; return 0; }
  if (forecast >= RAIN_SKIP_MM) { reason = "rain forecast"; return 0; }
  int minutes = min(MAX_RUN_MIN, (int)round((th + TARGET_OFFSET - m) * MIN_PER_POINT));
  reason = "dry, watering";
  return minutes;
}

void runValve(int minutes) {
  if (minutes <= 0) return;
  digitalWrite(RELAY_PIN, HIGH);
  unsigned long until = millis() + (unsigned long)minutes * 60000UL;
  while (millis() < until) {
    if (digitalRead(OVERRIDE_OFF_PIN) == LOW) break;   // human always wins
    delay(500);
  }
  digitalWrite(RELAY_PIN, LOW);
}

void logReading(float a, float b, float c, float m, int minutes, const String &reason) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(WEBHOOK_URL);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"s1\":" + String(a, 1) + ",\"s2\":" + String(b, 1) + ",\"s3\":" + String(c, 1) +
                ",\"median\":" + String(m, 1) + ",\"valve_min\":" + String(minutes) +
                ",\"reason\":\"" + reason + "\",\"crop_day\":" + String(cropDay) + "}";
  int code = http.POST(body);
  if (code == 200) {
    String resp = http.getString();             // dashboard may return {"forecast_mm":12}
    int i = resp.indexOf("forecast_mm");
    if (i >= 0) forecastMm = resp.substring(resp.indexOf(':', i) + 1).toFloat();
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(OVERRIDE_OFF_PIN, INPUT_PULLUP);
  pinMode(OVERRIDE_ON_PIN, INPUT_PULLUP);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void loop() {
  float a = readPercent(SENSOR_PINS[0]);
  float b = readPercent(SENSOR_PINS[1]);
  float c = readPercent(SENSOR_PINS[2]);
  float m = median3(a, b, c);
  String reason;
  int minutes = decide(m, cropDay, forecastMm, reason);
  Serial.printf("moisture %.1f%% -> %d min (%s)\n", m, minutes, reason.c_str());
  logReading(a, b, c, m, minutes, reason);
  runValve(minutes);
  delay(CYCLE_MS);
}
