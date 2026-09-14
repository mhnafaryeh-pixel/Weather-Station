#include "DHT.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#define SEALEVELPRESSURE_HPA (1013.25)
#define DHTPIN  4
#define DHTTYPE DHT22

const char* WIFI_SSID = "...";
const char* WIFI_PASS = ""...";
const char* MQTT_HOST = "...";
const int   MQTT_PORT = "...";
const char* MQTT_USER = "...";
const char* MQTT_PASS = "...";
const char* TOPIC_CMD  ="...";
const char* TOPIC_DATA = "...";

WiFiClientSecure net;
PubSubClient mqtt(net);

bool requestPending = false;
unsigned long lastAnswer = 0;
unsigned long lastPrint = 0;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

float h, t;

void readDHT() {
  h = dht.readHumidity();
  t = dht.readTemperature();

if (isnan(h) || isnan(t)) {
Serial.println(F("DHT22 : read failed"));
return;
  }

float hic = dht.computeHeatIndex(t, h, false);

Serial.print(F("DHT22 : "));
Serial.print(t, 1);
Serial.print(F(" C    "));
Serial.print(h, 1);
Serial.print(F(" %RH feels like "));
Serial.print(hic, 1);
Serial.println(F(" C"));

}

void readBMP() {
Serial.print(F("BMP280  : "));
Serial.print(bmp.readTemperature(), 1);
Serial.print(F(" C "));
Serial.print(bmp.readPressure() / 100.0F, 2);
Serial.print(F(" hPa    "));
Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA), 0);
Serial.println(F(" m"));
}

void dewPoint(){

if(h <= 0 || h > 100){
return;
}

float b = 17.62;
float c = 243.12;
float lambda = log(h / 100.0) + ((b * t) / (c + t));
float td =  (c * lambda) / (b - lambda);

Serial.print(F("DEW     : "));
Serial.print(td, 1);
Serial.println(F(" C"));

}

float dewValue() {
  if (isnan(h) || isnan(t) || h <= 0 || h > 100) return NAN;
  float b = 17.62;
  float c = 243.12;
  float lambda = log(h / 100.0) + ((b * t) / (c + t));

  return (c * lambda) / (b - lambda);
}

String payload;

void add(const char* key, float value, int digits) {
  if (isnan(value)) return;
  if (payload.length() > 1) payload += ",";
  payload += "\"";
  payload += key;
  payload += "\":";
  payload += String(value, digits);
}

String buildPayload() {
  payload = "{";
  add("temp",     t,                           1);
  add("hum",      h,                           1);
  add("dew",      dewValue(),                  1);
  add("temp2",    bmp.readTemperature(),       1);
  add("pressure", bmp.readPressure() / 100.0F, 2);
  payload += "}";
  return payload;
}

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print(F("WIFI    : connecting"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(400);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WIFI    : connected, IP "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("WIFI    : failed, will retry"));
  }
}

void onMessage(char* topic, byte* data, unsigned int len) {
  Serial.println(F("MQTT    : request received"));
  requestPending = true;
}

void ensureMqtt() {
  static unsigned long lastTry = 0;
  if (mqtt.connected()) return;
  if (millis() - lastTry < 5000) return;
  lastTry = millis();

  Serial.print(F("MQTT    : connecting... "));
  String id = "station-" + WiFi.macAddress();

  if (mqtt.connect(id.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println(F("connected"));
    mqtt.subscribe(TOPIC_CMD);
  } else {
    Serial.print(F("failed, state "));
    Serial.println(mqtt.state());
  }
}

void setup() {
Serial.begin(115200);
delay(2000);

dht.begin();
Wire.begin(21, 22);

if (!bmp.begin(0x76)) {
while (1) {
Serial.println(F("BMP280 not found - check wiring, CSB high, SDO low"));
delay(1000);
    }
  }

bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

Serial.println(F("station ready\n"));

  ensureWifi();
  net.setInsecure();                  
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setKeepAlive(30);
}

void loop() {

  ensureWifi();
  ensureMqtt();
  mqtt.loop();

  if (requestPending) {
    requestPending = false;

    if (millis() - lastAnswer > 5000) {     
      lastAnswer = millis();
      String msg = buildPayload();
      Serial.print(F("MQTT    : sending "));
      Serial.println(msg);
      mqtt.publish(TOPIC_DATA, msg.c_str());
    }
  }

if (millis() - lastPrint > 3000) {
lastPrint = millis();

readDHT();
readBMP();
dewPoint();
Serial.println();
  }
}
