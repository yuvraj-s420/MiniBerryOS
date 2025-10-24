/*
Description:

This is a multi application GUI with nested menus, live camera feed, reading/ writing to SD card, 
system data visualization using graphs (CPU, heap, PSRAM usage), and a collection of 3 simple games.

The system is meant to resemble an minimalist blackberry phone with limited features and simple controls

FreeRTOS is used to prioritize, suspend, and resume tasks to ensure a smooth user experience
TFT_eSPI library is used for pushing data to a IL9341 240x320 display

User input is obtained through the use of a single ADC pin connected to a resistor ladder with multiple buttons
This is done due to the limited GPIO pins left after camera, SD card, SPI TFT display pins are connected.

Author: Yuvraj Sandhu
*/

// ============================= Includes =============================
#include <Arduino.h>
#include <TFT_eSPI.h>           // Display library for TFT displays
#include "FS.h"                 // File system
#include "SD_MMC.h"             // SD Card ESP32
#include <WiFi.h>               // Wifi Connectivity
#include <time.h>               // NTP time and 
#include <esp_camera.h>         // Camera drivers
#include <vector>               // For filenames array that changes size
#include <queue>                // For holding ram usage over time and removing oldest

// My includes
#include "globals.h"            // All defines and global variables
#include "tasks.h"              // FreeRTOS
#include "button_handlers.h"    // Various button handling functions for different apps
#include "display.h"            // Drawing functions for TFT display
#include "helpers.h"            // General/ helper functions

void setup() {

  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);

  Serial.println("Booting...\n");

  // Initialization
  initQueues();
  initWiFi();
  initNTP();
  initSD();
  initTFT();
  initHeapQueue();
  loadFileNames();

  // Create tasks
  xTaskCreatePinnedToCore(
    displayTask,            // Task function
    "displayTask",          // Task name
    5000,                   // Stack size (bytes)
    NULL,                   // Task parameters
    3,                      // Priority
    NULL,                   // Task handle
    1                       // Core ID
  );
  Serial.println("displayTask initialized");

  xTaskCreatePinnedToCore(
    buttonTask,                   // Task function
    "buttonTask",                 // Task name
    5000,                         // Stack size (bytes)
    NULL,                         // Task parameters
    3,                            // Priority
    NULL,                         // Task handle
    1                             // Core ID
  );
  Serial.println("buttonTask initialized");

  xTaskCreatePinnedToCore(
    systemDataTask,               // Task function
    "systemDataTask",             // Task name
    5000,                         // Stack size (bytes)
    NULL,                         // Task parameters
    1,                            // Priority
    NULL,                         // Task handle
    1                             // Core ID
  );
  Serial.println("systemDataTask initialized");

  xTaskCreatePinnedToCore(
    wifiDataTask,                 // Task function
    "wifiDataTask",               // Task name
    5000,                         // Stack size (bytes)
    NULL,                         // Task parameters
    1,                            // Priority
    NULL,                         // Task handle
    1                             // Core ID
  );
  Serial.println("wifiDataTask initialized");

  xTaskCreatePinnedToCore(
    ntpTimeTask,                 // Task function
    "ntpTimeTask",               // Task name
    5000,                        // Stack size (bytes)
    NULL,                        // Task parameters
    1,                           // Priority
    NULL,                        // Task handle
    1                            // Core ID
  );
  Serial.println("ntpTimeTask initialized");

  // Suspend all tasks that won't be used at start
  vTaskSuspend(saveFrameToSDTask_handle);
  vTaskSuspend(deleteFromSDTask_handle);
  Serial.println("Suspended frame capture, save to sd, delete from sd tasks.");
}

void loop() {
  // Not needed as FreeRTOS handles tasks
}