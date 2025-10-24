# MiniBerryOS
Small scale GUI resembling an old BlackBerry phone with apps like camera, files, wifi, system data, and games. Coded in C++ within PlatformIO using FreeRTOS

[![Watch the video](https://img.youtube.com/vi/T4Agt9zRtqI/0.jpg)](https://youtube.com/shorts/T4Agt9zRtqI?feature=share)

## Features

- **FreeRTOS Multitasking:**  
  - **Frame Capture Task** – handles camera frame data  
  - **SD Task** – manages file I/O and logging  
  - **Display Task** – updates GUI elements and screen rendering  
  - **System Polling Task** – monitors CPU frequency, uptime, and memory usage
  - **NTP Polling Task** – syncs time with an online NTP server  
  - **Button Input Task** – reads and debounces input via voltage divider circuit  
  - **Wi-Fi Scan Task** – periodically checks and lists nearby Wi-Fi networks  

- **GUI Navigation:**  
  - Controlled by four buttons (Up, Down, Select, Back) using a voltage divider input  
  - Menu-driven interface with transitions between home screen and apps  

- **Built-in Apps:**  
  - **Camera:** view frames and save to SD card 
  - **Files:** browse SD card contents and delete files
  - **Wi-Fi:** connect/disconnect status and signal info  
  - **System Data:** live updating graph of heap usage (similar to task manager)
  - **Games:** simple catalogue of BlackBerry style games (Brick Breaker)

- **Optimized Rendering:**  
  - Double-buffered display to reduce flicker  
  - Lightweight graphics routines for 320×240 resolution  

- **Network Integration:**  
  - NTP-based time synchronization  
  - Background scanning for nearby access points  

---

## Hardware Setup

| Component | Description |
|------------|-------------|
| **MCU** | ESP32 WROVER |
| **Display** | 320×240 TFT LCD (SPI) |
| **Buttons** | 4x via voltage divider circuit (Up, Down, Select, Back) |
| **Storage** | SD card (SPI) |
| **Network** | Wi-Fi for connectivity and time sync |

---

## Build Instructions

```bash
git clone https://github.com/YuvrajSandhu/MiniBerryOS.git
cd MiniBerryOS
pio run --target upload
pio device monitor
