```markdown
# Wetterstation mit ESP32-C3

**Projekt:** SYT & ITP Wetterstation  
**Datum:** 27. Mai 2025  
**Verfasser:** Maik Gao & Marcel Hofmann  

---

## Inhaltsverzeichnis

1. [Einführung](#einführung)  
2. [Projektbeschreibung](#projektbeschreibung)  
3. [Hardware](#hardware)  
4. [Software & Bibliotheken](#software--bibliotheken)  
5. [Installation & Aufbau](#installation--aufbau)  
6. [Konfiguration](#konfiguration)  
7. [Verwendung](#verwendung)  
8. [Theoretische Grundlagen](#theoretische-grundlagen)  
9. [Code-Übersicht](#code-übersicht)  
10. [Zusammenfassung](#zusammenfassung)  
11. [Literaturverzeichnis](#literaturverzeichnis)  

---

## Einführung

Dieses Projekt realisiert eine voll funktionale Wetterstation auf Basis eines ESP32-C3 Mikrocontrollers. Erfasst werden Temperatur, Luftfeuchtigkeit und Magnetfeldstärke. Die Sensordaten werden auf einem lokalen OLED-Display angezeigt und über ein Webinterface (mit Chart.js) visualisiert. Eine RGB-Status-LED kann per Webinterface gesteuert werden.

---

## Projektbeschreibung

- **Sensoren**  
  - DHT11: Temperatur & Luftfeuchtigkeit  
  - Hall-Sensor (TTL/analog): Magnetfeldstärke  

- **Anzeige & Steuerung**  
  - OLED-Display (0,96″, 128×64) für lokale Darstellung  
  - Webinterface (ESPAsyncWebServer + Chart.js) für Live- und historische Daten  
  - RGB-LED (Neopixel) per Webinterface schaltbar  

- **Funktionen**  
  - Messintervalle (je 10 s) und Mittelwertbildung (5 Einzelmessungen)  
  - Ausreißererkennung  
  - Sleep Mode (Energiesparen)  
  - Wi-Fi-Manager für flexible WLAN-Anbindung  
  - NTP-Zeitsynchronisation  

---

## Hardware

| Komponente            | Typ/Modell                    | Anschluss am ESP32-C3      |
|-----------------------|-------------------------------|----------------------------|
| Mikrocontroller       | ESP32-C3 DevKitM1             | —                          |
| Temperatur/Feuchte    | DHT11                         | Daten → GPIO 2, 3.3 V, GND |
| Hall-Sensor           | TTL-/Analog-Hall              | Signal → GPIO 3, 3.3 V, GND|
| OLED-Display          | SSD1306 128×64                | SDA → GPIO 1, SCL → GPIO 10|
| RGB-LED (Neopixel)    | Adafruit NeoPixel (1 LED)     | Daten → GPIO 8             |
| Steckbrett & Kabel    | Jumperkabel, USB-Mini-Kabel   | —                          |

---

## Software & Bibliotheken

- **Arduino IDE** (ESP32-Boardpaket von Espressif)  
- **Bibliotheken**  
  - DHT sensor library & Adafruit Unified Sensor  
  - U8g2 (OLED-Anzeige)  
  - Adafruit NeoPixel  
  - ESPAsyncWebServer & AsyncTCP  
  - WiFiManager  
  - Chart.js (im Webinterface)  

---

## Installation & Aufbau

1. **Hardware verdrahten**  
   - Siehe Tabelle „Hardware“  
   - Pull-Up (10 kΩ) am DHT11-Datenpin  

2. **Arduino IDE einrichten**  
   - Board-Manager: „ESP32 by Espressif Systems“  
   - Bibliotheks-Manager: oben genannte Bibliotheken installieren  

3. **Sketch hochladen**  
   - öffnen: `Wetterstation.ino`  
   - Board → ESP32C3 Dev Module  
   - COM-Port wählen → Hochladen  

---

## Konfiguration

- Beim ersten Start öffnet ESP32 im AP-Modus `ESP_Config`  
- WLAN-Zugangsdaten über Captive Portal eingeben  
- Nach erfolgreicher Verbindung: NTP-Zeitbezug (`pool.ntp.org`)  

---

## Verwendung

1. **Webinterface aufrufen**  
   - IP-Adresse (im seriellen Monitor ersichtlich) in Browser eingeben  
2. **Live-Daten & Graph**  
   - Aktuelle Sensorwerte  
   - Historisches Liniendiagramm (letzte 10 Minuten)  
3. **LED steuern**  
   - Checkbox „Status-LED“ toggeln → per GET-Request `/led?state=on|off`  

---

## Theoretische Grundlagen

- **Sensorik & Datenaggregation**  
  - Mittelwerte (je 5 Messungen) zur Reduktion von Ausreißern  
  - Speicherung in Arrays (max. 50 Datenpunkte)  

- **Webserver-Programmierung**  
  - Asynchroner Webserver (ESPAsyncWebServer)  
  - AJAX-Polling (alle 5 s) für JSON-Datenendpunkt  

- **Energieeffizienz**  
  - Sleep Mode des ESP32 zur Stromersparnis  
  - Display-Updates und Messintervalle optimiert  

---

## Code-Übersicht

- **Setup**  
  - `WiFiManager autoConnect()` → Captive Portal  
  - `configTime()` → NTP  
  - `setupWebServer()` → Routing für `/` (HTML) und `/data` (JSON)  

- **Loop**  
  - `analogRead(HALLPIN)`  
  - `averageTemperature()`, `averageHumidity()`  
  - LED-Farbsteuerung je nach Temperatur  
  - Display-Update (1 s) & Messung (10 s)  
  - Historische Datenpflege (Ringpuffer)  

> **Hinweis:** Den vollständigen Sketch finden Sie im Ordner `src/` als `Wetterstation.ino`.

---

## Zusammenfassung

Dieses Projekt zeigt den kompletten Workflow von der Hardware-Auswahl über Sensorintegration bis hin zur Web-Visualisierung. Alle Komponenten lassen sich mit Grundkenntnissen der Arduino-IDE und Mikrocontrollerprogrammierung nachbauen und erweitern.

---

## Literaturverzeichnis

1. Arduino – DHT11 Tutorial. *Arduino Getting Started.*  
2. Chart.js Dokumentation. *W3Schools.*  
3. ESP32 OLED Tutorial. *esp32io.com.*  
4. Random Nerd Tutorials – ESP32 & DHT11.  
5. Tiago. ESP32-C3 Super Mini RTClock mit OLED.  
6. ESPAsyncWebServer. GitHub.  
7. tzapu/WiFiManager. GitHub.  
8. Random Nerd Tutorials – WiFiManager & AsyncWebServer.  

---
```
