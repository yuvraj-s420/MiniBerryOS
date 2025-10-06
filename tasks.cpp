#include <Arduino.h>
#include <esp_camera.h>
#include <WiFi.h>
#include <SD_MMC.h>
#include "tasks.h"
#include "display.h"
#include "helpers.h"
#include "button_handlers.h"


void buttonTask(void* parameter){
  // Task that reads analog input from resistor ladder and assigns a corresponding button
  for (;;){

    ButtonState state;
    int reading = analogRead(BUTTON_PIN);   //(0-4095)

    // Catch reading and update button state
    if (reading < 100){             // Pulldown resistor gives reading of 0, but to be safe, made it less than 100
        state = NONE;
    }
    else if (reading <= UP_ADC){
        state = UP;
    }
    else if (reading <= DOWN_ADC){
        state = DOWN;
    }
    else if (reading <= SELECT_ADC){
        state = SELECT;
    }
    else if (reading <= BACK_ADC){
        state = BACK;
    }
    xQueueSend(button_queue, &state, 0);                  // Overwrite button state to queue
    
    //Serial.printf("buttonTask high watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));        
    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_INTERVAL));         // Debounce button inputs and yield CPU
  }
}

void frameCaptureTask(void* parameter){
  // Task that gets frame from the camera
  // Then sends frame to camera queue for display
  // Sends to save queue if save flag is true

  for (;;){

    camera_fb_t *fb = esp_camera_fb_get();            // Get frame buffer from camera

    //transposeImageInPlace(fb);                       

    if (fb) {
      // Send to display queue (always keep latest)
      xQueueSend(frame_display_queue, &fb, 0);      // Dont block if queue is full (check what happens if you do xQueueOverwrite)

      // Send to save queue (only if flag is true)
      if (save_next_frame){
        xQueueSend(frame_save_queue, &fb, 0); // Non-blocking, skip if full
      }
    }
    else {
      Serial.println("Could not get frame!");   
    }

    //Serial.printf("frameCaptureTask high watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));        

    vTaskDelay(pdMS_TO_TICKS(33));                 // Cap frame captures at 30HZ
  }
}

void systemDataTask(void *parameter) {
  // Gets system data like free heap, psram, cpu frequency, etc every 2s
  // Sends to queue
  for (;;) {
    SystemInfo info;

    info.min_free_heap = ESP.getMinFreeHeap();
    info.cpu_freq = getCpuFrequencyMhz();
    info.uptime = millis() / 1000; // uptime in seconds

    // Log Heap usage in kB
    int used = (ESP.getHeapSize() - ESP.getFreeHeap()) / 1000;
    if (used > max_heap){
      max_heap = used;
    }

    if (heap_usage.size() > NUM_SYS_DATA_POINTS){
      heap_usage.pop();                               // Remove oldest element
    } else{
      heap_usage.push(used);
    }
    
    // Send system info
    xQueueOverwrite(sys_info_queue, &info);

    //Serial.printf("systemDataTask high watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));  

    vTaskDelay(pdMS_TO_TICKS(1000));  // Update every 2s
  }
}

void wifiDataTask(void *parameter) {
  // Gets wifi data like home wifi SSID, IP, RSSI, mac address
  // As well as data about nearby wifi signals
  // Sends to queue
  for (;;) {
    WiFiInfo info = {};

    // Fill home WiFi info
    const String ss = WiFi.SSID();
    ss.toCharArray(info.ssid, sizeof(info.ssid));
    WiFi.localIP().toString().toCharArray(info.ip, sizeof(info.ip));
    info.rssi = WiFi.RSSI();
    WiFi.macAddress().toCharArray(info.mac, sizeof(info.mac));

    // Scan (this is blocking and can take a few hundred ms thus in a seperate task)
    int n = WiFi.scanNetworks();
    if (n > 0) {
      info.nearbyCount = min(n, MAX_WIFI);
      for (int i = 0; i < info.nearbyCount; ++i) {
        String tmp = WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
        tmp.toCharArray(info.nearby[i], sizeof(info.nearby[i]));
      }
    } else {
      info.nearbyCount = 0;
    }

    xQueueOverwrite(wifi_queue, &info);

    //Serial.printf("wifiDataTask high watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));  

    vTaskDelay(pdMS_TO_TICKS(5000));  // refresh every 5s
  }
}

void ntpTimeTask(void *parameter){
  // Sets the global time variable

  for (;;){
    getLocalTime(&t);

    //Serial.printf("ntpTimeTask high watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));  

    vTaskDelay(pdMS_TO_TICKS(1000));        // Every second
  }
}

void saveFrameToSDTask(void* parameter) { 
  // Saves the frame buffer in frame_save_queue once it gets it
  // If no frame was requested to be saved, queue would be empty
  // SD writes are time consuming, so it is important this happens independent of display task
    
  for (;;) {

    camera_fb_t *fb;

    if (xQueueReceive(frame_save_queue, &fb, 0)) {  

      char f[32] = {0};
      sprintf(f, "/%04d-%02d-%02d_%02d-%02d-%02d.raw",      // Save as .raw for raw RGB565 pixel values (easy to work with and display in file viewer)
              t.tm_year + 1900,                          // Format filename as current date and time
              t.tm_mon + 1,
              t.tm_mday,
              t.tm_hour,
              t.tm_min,
              t.tm_sec);

      // This conversion causes a lot of issues. 
      // Using char arrays works, but converting to string or displaying string gives junk names
      //String filename(f);
      //Serial.println(f);                      worked
      //Serial.println(String(f));              worked
      //Serial.printf("Photo saved as filename: %s", filename);   shows junk

      fs::File file = SD_MMC.open(f, FILE_WRITE);        // Create file in SD

      if (file){
        file.write(fb->buf, fb->len);                           // Write buffer to file
        file.close();

        Serial.printf("Photo saved as filename: %s", f);        // Works with f, not with String(filename)

        save_next_frame = false;


      } else{
        Serial.println("failed to open file for writing!");
      }
      esp_camera_fb_return(fb);                               // Return fb
    }

    //Serial.printf("saveFrameToSDTask high watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));  

    vTaskDelay(pdMS_TO_TICKS(200));                         // Release cpu for other tasks and cap save rate at 5 times a second
  }
}

void deleteFromSDTask(void* parameter){
  // Task that deletes files recieved from the queue when they arrive.
  // SD reads, writes and deletes are time consuming, so it is important this happens independent of display task
  for (;;){
    char filename[MAX_FILENAME_LENGTH];

    if (xQueueReceive(file_delete_queue, &filename, 0)){

      //Serial.println(filenames[file_index]);
      Serial.println(filename);         // Now that we are sending char arrays and NOT STRINGS, it works

      if (SD_MMC.remove(filename)){
        Serial.printf("Successfully deleted file: %s\n", filename);
        loadFileNames();        // Reload file names and counts after deleting file
      }
      else{
        Serial.printf("Failed to delete file: %s\n", filename);
      }
    }

    //Serial.printf("deleteFromSDTask high watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));  

    vTaskDelay(pdMS_TO_TICKS(100));                // Free cpu for other tasks (frame saves happen relatively slowly anyways)
  }
}

void displayTask(void* parameter){
  // Task that displays the GUI

  for (;;){

    now = millis();

    switch (display_state){
      case BOOT:                                  // Display Boot animation and switch to menu state

        drawBoot();
        vTaskDelay(pdMS_TO_TICKS(2000));    // Delay task until boot animation has finished
        display_state = MENU;
        tft.fillScreen(TFT_BLACK);
        break;

      case MENU:                    // Display Menu Items
        
        if (!menu_init){            // Draw menu once, update highlight each time to increase performance
          tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          drawMenu(main_menu, sizeof(main_menu) / sizeof(MenuItem));
          menu_init = true;
          
        }
        drawStatusBar();
        updateMenuHighlight(main_menu, sizeof(main_menu) / sizeof(MenuItem));
        handleButtonMenu(main_menu, sizeof(main_menu) / sizeof(MenuItem));
        break;

      case CAMERA_FEED:             // Display camera feed with options
        
        if (!camera_init){    // Init camera once when entering camera feed
          tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          esp_err_t err = esp_camera_init(&camera_config);
          if (err != ESP_OK){
            Serial.printf("Camera could not initialize due to error 0x%x\n", err);
          }
          xTaskCreatePinnedToCore(
            frameCaptureTask,             // Task function
            "frameCaptureTask",           // Task name
            10000,                        // Stack size (bytes)
            NULL,                         // Task parameters
            3,                            // Priority
            &frameCaptureTask_handle,     // Task handle
            1                             // Core ID
          );
          xTaskCreatePinnedToCore(
            saveFrameToSDTask,            // Task function
            "saveFrameToSDTask",          // Task name
            5000,                         // Stack size (bytes)
            NULL,                         // Task parameters
            1,                            // Priority
            &saveFrameToSDTask_handle,    // Task handle
            1                             // Core ID
          );
          camera_init = true;
          save_next_frame = false;
          drawCameraButton();                         // Draw once at start to prevent flicker
        }
        drawStatusBar();
        drawCameraFeed();                             // Draws frames to screen as they come, with option to save to SD (might add more DSP options)
        handleButtonCamera();     
                           
        break;

      case SD_CARD:           // Display file viewer, or image in file 

        if (!menu_init){      // Load filenames once when entering SD Card viewer (also loads after file gets deleted in delete task)
          tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          xTaskCreatePinnedToCore(
            deleteFromSDTask,             // Task function
            "deleteFromSDTask",           // Task name
            5000,                         // Stack size (bytes)
            NULL,                         // Task parameters
            1,                            // Priority
            &deleteFromSDTask_handle,     // Task handle
            1                             // Core ID
          );
          loadFileNames();
          printFileNames();
          menu_init = true;
        }

        drawStatusBar();
        if (!image_view){     // Draws file selector with filenames
          drawFiles();
        }
        else{                 
          drawImageViewer();      // Draws image saved in file with option to delete
        }
        handleButtonFiles();

        break;

      case SYSTEM_DATA:             // Display system data like heap, psram, cpu usage (Will update to show data in a graph over total runtime)

        if(!menu_init){
          tft.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          menu_init = true;
        }
        drawStatusBar();
        drawSystemData();
        handleButtonSimple();
        break;

        case WIFI:                  // Display WIFI connectivity info and other nearby wifi signals

        if(!menu_init){
          tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          menu_init = true;
        }
        drawStatusBar();
        drawWifi();
        handleButtonSimple();
        
        break;

      case GAMES:                   // Display menu for all avaliable games
        
        if (!menu_init){            // Draw menu once, update highlight each time to increase performance
          tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          drawMenu(games_menu, sizeof(games_menu) / sizeof(MenuItem));
          menu_init = true;
          
        }
        drawStatusBar();
        updateMenuHighlight(games_menu, sizeof(games_menu) / sizeof(MenuItem));
        handleButtonMenu(games_menu, sizeof(games_menu) / sizeof(MenuItem));

        break;

      case GAME1: {                               // Brick Breaker

        static char res = ' ';
        static TFT_eSprite paddle(&tft);
        static TFT_eSprite ball(&tft);
        const int num_bricks = 16;
        //static TFT_eSprite bricks[]

        if(!menu_init){
          tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          initGame1(&paddle, &ball, NULL);
          menu_init = true;
        }

        drawStatusBar();
        res = handleButtonGame(); 
        updateGame1(&paddle, &ball, NULL, res);
        
        break;
      }

      case GAME2:                               // Flappy Bird

        if(!menu_init){
          tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          menu_init = true;
        }
        drawStatusBar();
        drawGame2();
        handleButtonGame();
        break;
      
      case GAME3:

        if(!menu_init){
          tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
          menu_init = true;
        }

        drawStatusBar();
        drawGame3();
        handleButtonGame();
        break;
        
      default:
        display_state = MENU;
        break;
    }

    // Calculate frame rate for top status bar
    frames++;   // Increment frames after drawing
    if (now - before >= 1000){    // Calculate fps every second
      fps = frames / 1000.f * (now - before);
      frames = 0;
      before = now;
    }

    //Serial.printf("displayTask high watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));  

    vTaskDelay(pdMS_TO_TICKS(10));    // Cap at 100 fps
    }  
}


