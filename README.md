# ESP32 Smart Terminal with NVS-backed Device Identification

This project implements a PC–ESP32 UART communication system using ESP-IDF.
The ESP32 performs device identification and stores user-provided strings
persistently in NVS flash. A PC-side application acts as a smart terminal,
selectively requesting and retrieving only relevant data from the ESP32.

## Features
- ESP-IDF based firmware (FreeRTOS)
- UART-based PC ↔ ESP32 communication
- Device identification mechanism
- Persistent storage using NVS
- Selective UART data filtering (smart terminal behavior)

## Build & Flash
```bash
idf.py build
idf.py flash monitor
