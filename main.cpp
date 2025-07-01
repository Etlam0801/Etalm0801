
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_dmx.h>
// ==================== KONFIGURATION ====================
// WLAN Einstellungen
const char* WIFI_SSID = "Dein_W-LAN";
const char* WIFI_PASSWORD = "W-LAN-PW";
// MQTT Einstellungen
const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "NAME _DMX_Controller";

// MQTT Topics (für 5 DMX-Kanäle)
const char* MQTT_TOPICS[5] = {
  "NAME/dmx/channel1",
  " NAME/dmx/channel2",
  " NAME/dmx/channel3",
  " NAME/dmx/channel4",
  " NAME/dmx/channel5"
};
// DMX Einstellungen - Korrekte UART1 Pins verwenden!
const int DMX_TX_PIN = 17; // GPIO 17 - UART1 TX (Standard für DMX)
const int DMX_RX_PIN = 16; // GPIO 16 - UART1 RX (für vollständige UART Funktion)
const int DMX_EN_PIN = 4;  // GPIO 4  - Enable Pin (DE/RE)
const int DMX_CHANNELS[5] = {1, 2, 3, 4, 5};
dmx_port_t dmx_num = DMX_NUM_1;  // Mögliche Werte: DMX_NUM_0, DMX_NUM_1, DMX_NUM_2
QueueHandle_t dmx_queue;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
int dmxValues[5] = {0, 0, 0, 0, 0};

// Test-Funktion für DMX ohne MQTT
void testDMX() {
  Serial.println("=== DMX TEST START ===");
  uint8_t dmx_data[DMX_PACKET_SIZE] = {0};
  
  // Teste alle 5 Kanäle mit 50% Helligkeit
  for (int i = 0; i < 5; i++) {
    dmx_data[DMX_CHANNELS[i]] = 128; // 50% von 255
    Serial.print("Setze DMX Kanal ");
    Serial.print(DMX_CHANNELS[i]);
    Serial.println(" auf 50%");
  }
  
  size_t written = dmx_write(dmx_num, dmx_data, DMX_PACKET_SIZE);
  size_t sent = dmx_send(dmx_num);
  
  Serial.print("Test - Geschrieben: ");
  Serial.print(written);
  Serial.print(" Bytes, Gesendet: ");
  Serial.print(sent);
  Serial.println(" Bytes");
  Serial.println("=== DMX TEST ENDE ===");
}
// ==================== FUNKTIONEN ====================
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Verbinde mit WLAN: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WLAN verbunden!");
  Serial.print("IP Adresse: ");
  Serial.println(WiFi.localIP());
}
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Verbinde mit MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println(" verbunden!");
      for (int i = 0; i < 5; i++) {
        mqttClient.subscribe(MQTT_TOPICS[i]);
        Serial.print("Abonniert: ");
        Serial.println(MQTT_TOPICS[i]);
      }
    } else {
      Serial.print(" fehlgeschlagen, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Versuche erneut in 5 Sekunden");
      delay(5000);
    }
  }
}
void setupDMX() {
  dmx_config_t config = {
    .interrupt_flags = ESP_INTR_FLAG_IRAM
  };
  
  Serial.print("Installiere DMX Driver auf Port ");
  Serial.println(dmx_num);
  bool driver_ok = dmx_driver_install(dmx_num, &config, 0, 0);
  Serial.print("DMX Driver Installation: ");
  Serial.println(driver_ok ? "OK" : "FEHLER");
  
  Serial.print("Setze DMX Pins - TX:");
  Serial.print(DMX_TX_PIN);
  Serial.print(", RX:");
  Serial.print(DMX_RX_PIN);
  Serial.print(", EN:");
  Serial.println(DMX_EN_PIN);
  bool pin_ok = dmx_set_pin(dmx_num, DMX_TX_PIN, DMX_RX_PIN, DMX_EN_PIN);
  Serial.print("DMX Pin Setup: ");
  Serial.println(pin_ok ? "OK" : "FEHLER");
  
  Serial.println("DMX initialisiert");
  // DMX-Daten zurücksetzen
  uint8_t dmx_data[DMX_PACKET_SIZE] = {0};
  for (int i = 0; i < 5; i++) {
    if (DMX_CHANNELS[i] < DMX_PACKET_SIZE)
      dmx_data[DMX_CHANNELS[i]] = 0;
  }
  size_t written = dmx_write(dmx_num, dmx_data, DMX_PACKET_SIZE);
  Serial.print("DMX Daten geschrieben: ");
  Serial.print(written);
  Serial.println(" Bytes");
  
  size_t sent = dmx_send(dmx_num);
  Serial.print("DMX Paket gesendet: ");
  Serial.print(sent);
  Serial.println(" Bytes");
}
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("MQTT empfangen - Topic: ");
  Serial.print(topic);
  Serial.print(", Nachricht: ");
  Serial.println(message);
  
  int value = message.toInt();
  value = constrain(value, 0, 100);
  
  for (int i = 0; i < 5; i++) {
    if (String(topic) == MQTT_TOPICS[i]) {
      dmxValues[i] = value;
      int dmxValue = map(value, 0, 100, 0, 255);
      
      Serial.print("Verarbeite Kanal ");
      Serial.print(i + 1);
      Serial.print(" - DMX Adresse: ");
      Serial.print(DMX_CHANNELS[i]);
      Serial.print(", Wert: ");
      Serial.print(value);
      Serial.print("% -> DMX: ");
      Serial.println(dmxValue);
      
      uint8_t dmx_data[DMX_PACKET_SIZE];
      size_t read_bytes = dmx_read(dmx_num, dmx_data, DMX_PACKET_SIZE);
      Serial.print("DMX Daten gelesen: ");
      Serial.print(read_bytes);
      Serial.println(" Bytes");
      
      if (DMX_CHANNELS[i] < DMX_PACKET_SIZE) {
        dmx_data[DMX_CHANNELS[i]] = dmxValue;
        size_t written = dmx_write(dmx_num, dmx_data, DMX_PACKET_SIZE);
        Serial.print("DMX Daten geschrieben: ");
        Serial.print(written);
        Serial.println(" Bytes");
        
        size_t sent = dmx_send(dmx_num);
        Serial.print("DMX Paket gesendet: ");
        Serial.print(sent);
        Serial.println(" Bytes");
      }
      
      Serial.print("DMX Kanal ");
      Serial.print(DMX_CHANNELS[i]);
      Serial.print(" auf ");
      Serial.print(value);
      Serial.print("% (DMX: ");
      Serial.print(dmxValue);
      Serial.println(")");
      break;
    }
  }
}
// ==================== HAUPTPROGRAMM ====================
void setup() {
  Serial.begin(115200);
  Serial.println("DMX MQTT Controller startet...");
  setupWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  setupDMX();
  
  // 5 Sekunden warten, dann DMX Test
  delay(5000);
  testDMX();
  
  Serial.println("Setup abgeschlossen!");
}
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WLAN Verbindung verloren, verbinde neu...");
    setupWiFi();
  }
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  delay(10);
}


