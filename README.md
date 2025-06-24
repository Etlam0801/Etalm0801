# ESP32 DMX MQTT Controller

Ein ESP32-basierter DMX-Controller, der über MQTT gesteuert werden kann. Dieses Projekt ermöglicht die Fernsteuerung von DMX-Geräten über WLAN und MQTT.

## Features

- Steuerung von 5 DMX-Kanälen über MQTT
- Automatische Verbindung mit WLAN und MQTT-Broker
- Prozentuale Steuerung (0-100%) wird in DMX-Werte (0-255) umgerechnet
- Serielle Debug-Ausgabe

## Hardware

- ESP32 Entwicklungsboard
- MAX485 oder ähnlicher DMX-Treiber
- 3-Pin XLR-Buchse für DMX-Ausgang

## Pinbelegung

- DMX TX: GPIO2
- DMX EN: GPIO4

## Software-Abhängigkeiten

- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [ESP DMX](https://github.com/someweisguy/esp_dmx)

## Installation

1. Klone dieses Repository
2. Öffne das Projekt in PlatformIO
3. Kopiere `include/config.h.example` nach `include/config.h` und passe deine WLAN- und MQTT-Einstellungen an
4. Kompiliere und lade das Programm auf deinen ESP32

## MQTT-Topics

- `dmx/channel1` - DMX Kanal 1 (Werte: 0-100)
- `dmx/channel2` - DMX Kanal 2 (Werte: 0-100)
- `dmx/channel3` - DMX Kanal 3 (Werte: 0-100)
- `dmx/channel4` - DMX Kanal 4 (Werte: 0-100)
- `dmx/channel5` - DMX Kanal 5 (Werte: 0-100)

## Lizenz

MIT