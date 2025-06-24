// filepath: /ESP32-DMX-MQTT-Controller/ESP32-DMX-MQTT-Controller/src/main.cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_dmx.h>

// ==================== KONFIGURATION ====================
// WLAN Einstellungen
const char* WIFI_SSID = "villakunterbunt";
const char* WIFI_PASSWORD = "2mal3=KleinerOnkel";

// MQTT Einstellungen
const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "FTA1_DMX_Controller";

// MQTT Topics (für 5 DMX-Kanäle)
const char* MQTT_TOPICS[5] = {
  "dmx/channel1",
  "dmx/channel2",
  "dmx/channel3",
  "dmx/channel4",
  "dmx/channel5"
};

// DMX Einstellungen
const int DMX_TX_PIN = 2;
const int DMX_RX_PIN = -1;
const int DMX_EN_PIN = 4;
const int DMX_CHANNELS[5] = {1, 2, 3, 4, 5};

dmx_port_t dmx_num = DMX_NUM_1;
QueueHandle_t dmx_queue;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
int dmxValues[5] = {0, 0, 0, 0, 0};

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

  dmx_driver_install(dmx_num, &config, 0, 0);
  dmx_set_pin(dmx_num, DMX_TX_PIN, DMX_RX_PIN, DMX_EN_PIN);

  Serial.println("DMX initialisiert");

  // DMX-Daten zurücksetzen
  uint8_t dmx_data[DMX_PACKET_SIZE] = {0};
  for (int i = 0; i < 5; i++) {
    if (DMX_CHANNELS[i] < DMX_PACKET_SIZE)
      dmx_data[DMX_CHANNELS[i]] = 0;
  }

  dmx_write(dmx_num, dmx_data, DMX_PACKET_SIZE);
  dmx_send(dmx_num);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  int value = message.toInt();
  value = constrain(value, 0, 100);

  for (int i = 0; i < 5; i++) {
    if (String(topic) == MQTT_TOPICS[i]) {
      dmxValues[i] = value;
      int dmxValue = map(value, 0, 100, 0, 255);

      uint8_t dmx_data[DMX_PACKET_SIZE];
      dmx_read(dmx_num, dmx_data, DMX_PACKET_SIZE);

      if (DMX_CHANNELS[i] < DMX_PACKET_SIZE) {
        dmx_data[DMX_CHANNELS[i]] = dmxValue;
        dmx_write(dmx_num, dmx_data, DMX_PACKET_SIZE);
        dmx_send(dmx_num);
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