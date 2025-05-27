#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <U8g2lib.h>
#include "DHT.h"
#include "time.h"
#include <Adafruit_NeoPixel.h>

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define HALLPIN 3
#define NEOPIXEL_PIN 8
#define NUMPIXELS 1
Adafruit_NeoPixel pixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
bool rgbEnabled = true;

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 10, 1, U8X8_PIN_NONE); // SCL=10, SDA=1

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

AsyncWebServer server(80);

float lastTemp = NAN;
float lastHum = NAN;
int hallValue = 0;

unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 1000;
unsigned long lastMeasurement = 0;
const unsigned long measurementInterval = 10000;

#define MAX_DATA_POINTS 50
String labels[MAX_DATA_POINTS];
float tempData[MAX_DATA_POINTS];
float humData[MAX_DATA_POINTS];
int dataIndex = 0;

float averageTemperature() {
  float sum = 0;
  int count = 0;
  for(int i = 0; i < 5; i++) {
    float t = dht.readTemperature();
    if(!isnan(t) && t > -40 && t < 80) {
      sum += t;
      count++;
    }
    delay(100);
  }
  return count > 0 ? sum / count : NAN;
}

float averageHumidity() {
  float sum = 0;
  int count = 0;
  for(int i = 0; i < 5; i++) {
    float h = dht.readHumidity();
    if(!isnan(h) && h >= 0 && h <= 100) {
      sum += h;
      count++;
    }
    delay(100);
  }
  return count > 0 ? sum / count : NAN;
}

void setPixelColor(uint8_t r, uint8_t g, uint8_t b) {
  if(!rgbEnabled) {
    pixel.clear();
    pixel.show();
    return;
  }
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void updateDisplay(struct tm timeinfo) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  u8g2.drawStr(0, 12, timeStr);
  String tempStr = "Temp: " + String(lastTemp, 1) + " C";
  String humStr = "Feuchte: " + String(lastHum, 1) + " %";
  String hallStr = "Hall: " + String(hallValue);
  u8g2.drawStr(0, 30, tempStr.c_str());
  u8g2.drawStr(0, 45, humStr.c_str());
  u8g2.drawStr(0, 60, hallStr.c_str());
  u8g2.sendBuffer();
}

String createHTML() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="utf-8">
    <title>ESP32 Wetterstation</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
      body {
        font-family: 'Segoe UI', sans-serif;
        background: #f0f4f8;
        color: #333;
        text-align: center;
        padding: 20px;
      }
      h1 {
        color: #2c3e50;
      }
      #values {
        background: #fff;
        padding: 15px;
        border-radius: 10px;
        box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        max-width: 400px;
        margin: 20px auto;
      }
      #values p {
        font-size: 1.1em;
        margin: 10px 0;
      }
      label {
        display: inline-block;
        margin-bottom: 15px;
        font-weight: bold;
      }
      input[type="checkbox"] {
        transform: scale(1.3);
        margin-left: 10px;
      }
      canvas {
        max-width: 100%;
        background: #fff;
        border-radius: 10px;
        box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        padding: 10px;
        margin-top: 20px;
      }
    </style>
  </head>
  <body>
    <h1>ESP32 Wetterstation</h1>
    <label>Status-LED:
      <input type="checkbox" id="ledToggle" checked onchange="toggleLED(this.checked)">
    </label>
    <div id="values">
      <p><b>Aktuelle Temperatur:</b> <span id="temp">--</span> °C</p>
      <p><b>Aktuelle Luftfeuchtigkeit:</b> <span id="hum">--</span> %</p>
      <p><b>Aktueller Hall-Wert:</b> <span id="hall">--</span></p>
    </div>
    <canvas id="tempChart" width="400" height="200"></canvas>
    <script>
    let tempChart;
    async function fetchData() {
      try {
        const res = await fetch('/data');
        const json = await res.json();
        document.getElementById("temp").innerText = json.temp.toFixed(1);
        document.getElementById("hum").innerText = json.hum.toFixed(1);
        document.getElementById("hall").innerText = json.hall;
        if(!tempChart) {
          const ctx = document.getElementById('tempChart').getContext('2d');
          tempChart = new Chart(ctx, {
            type: 'line',
            data: {
              labels: json.labels,
              datasets: [
                {
                  label: 'Temperatur (°C)',
                  data: json.temps,
                  borderColor: 'crimson',
                  backgroundColor: 'rgba(220, 20, 60, 0.1)',
                  fill: true,
                  tension: 0.3
                },
                {
                  label: 'Feuchtigkeit (%)',
                  data: json.hums,
                  borderColor: 'dodgerblue',
                  backgroundColor: 'rgba(30, 144, 255, 0.1)',
                  fill: true,
                  tension: 0.3
                }
              ]
            },
            options: {
              responsive: true,
              animation: false,
              scales: {
                y: {
                  beginAtZero: true
                }
              },
              plugins: {
                legend: {
                  labels: {
                    color: '#333'
                  }
                }
              }
            }
          });
        } else {
          tempChart.data.labels = json.labels;
          tempChart.data.datasets[0].data = json.temps;
          tempChart.data.datasets[1].data = json.hums;
          tempChart.update();
        }
      } catch (error) {
        console.error('Fehler beim Laden der Daten:', error);
      }
    }
    function toggleLED(state) {
      fetch('/led?state=' + (state ? 'on' : 'off'));
    }
    fetchData();
    setInterval(fetchData, 5000);
  </script>
  </body>
  </html>
  )rawliteral";
  return html;
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", createHTML());
  });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest* request) {
    String json = "{\"labels\":[";
    for(int i = 0; i < dataIndex; i++) {
      json += "\"" + labels[i] + "\"";
      if(i < dataIndex - 1) {
        json += ",";
      }
    }
    json += "],\"temps\":[";
    for(int i = 0; i < dataIndex; i++) {
      json += String(tempData[i], 1);
      if(i < dataIndex - 1) {
        json += ",";
      }
    }
    json += "],\"hums\":[";
    for(int i = 0; i < dataIndex; i++) {
      json += String(humData[i], 1);
      if(i < dataIndex - 1) {
        json += ",";
      }
    }
    json += "],";
    json += "\"temp\":" + String(lastTemp, 1) + ",";
    json += "\"hum\":" + String(lastHum, 1) + ",";
    json += "\"hall\":" + String(hallValue);
    json += "}";
    request->send(200, "application/json", json);
  });
  server.on("/led", HTTP_GET, [](AsyncWebServerRequest* request) {
    if(request->hasParam("state")) {
      String state = request->getParam("state")->value();
      rgbEnabled = (state == "on");
    }
    request->send(200, "text/plain", rgbEnabled ? "on" : "off");
  });
  server.begin();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(HALLPIN, INPUT);
  pixel.begin();
  pixel.setBrightness(50);
  pixel.show();
  u8g2.begin();
  WiFiManager wm;
  if(!wm.autoConnect("ESP_Config")) {
    setPixelColor(255, 0, 0); // rot = kein WLAN
    delay(1000);
    ESP.restart();
  }
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
  }
  setupWebServer();
}

void loop() {
  hallValue = analogRead(HALLPIN);
  setPixelColor(0, 0, 255); // Messung im Gange
  float temperature = averageTemperature();
  float humidity = averageHumidity();
  if(!WiFi.isConnected()) {
    setPixelColor(255, 0, 0); // rot
  } else if(temperature > 28.0) {
    setPixelColor(255, 165, 0); // orange
  } else {
    setPixelColor(0, 255, 0); // grün
  }
  unsigned long nowMillis = millis();
  if(nowMillis - lastDisplayUpdate > displayUpdateInterval) {
    lastDisplayUpdate = nowMillis;
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)) {
      updateDisplay(timeinfo);
    }
  }
  if(nowMillis - lastMeasurement > measurementInterval) {
    lastMeasurement = nowMillis;
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
      return;
    }
    if(isnan(temperature) || isnan(humidity)) {
      return;
    }
    lastTemp = temperature;
    lastHum = humidity;
    char label[10];
    strftime(label, sizeof(label), "%H:%M", &timeinfo);
    if(dataIndex < MAX_DATA_POINTS) {
      labels[dataIndex] = String(label);
      tempData[dataIndex] = temperature;
      humData[dataIndex] = humidity;
      dataIndex++;
    } else {
      for(int i = 1; i < MAX_DATA_POINTS; i++) {
        labels[i - 1] = labels[i];
        tempData[i - 1] = tempData[i];
        humData[i - 1] = humData[i];
      }
      labels[MAX_DATA_POINTS - 1] = String(label);
      tempData[MAX_DATA_POINTS - 1] = temperature;
      humData[MAX_DATA_POINTS - 1] = humidity;
    }
  }
}