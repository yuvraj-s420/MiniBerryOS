/*

Global defines, structs, data types, variables, etc.

*/

#pragma once
#include <Arduino.h>
#include <esp_camera.h>
#include <TFT_eSPI.h>
#include <vector>
#include <queue>
#include "config.h"
#include "time.h"

// ============================= Defines =============================
// TFT display defines
/* 
The following defines are for the pin mappings of the TFT, 
and need to be changed in User_Setup.h within the TFT_eSPI library

TFT_MISO                Not used
#define TFT_MOSI 12
#define TFT_SCLK 13
TFT_CS                  Connected to GND to always be on    
#define TFT_DC   33 
#define TFT_RST  -1     Set TFT_RST to -1 if display RESET is connected to ESP32 board RST
*/
#define SCREEN_HEIGHT 320
#define SCREEN_WIDTH 240
#define STATUS_BAR_HEIGHT 20
#define MENU_ITEM_HEIGHT 60
#define IMAGE_WIDTH 240
#define IMAGE_HEIGHT 240
#define SWAP_BYTES false          // For little/ big endian byte swaps (Instead of RRRRRGGG GGGBBBBB, LSB first: GGGBBBBB RRRRRGGG )


// Singular ADC pin used in conjunction with resistor ladder to encode different button inputs
#define BUTTON_PIN 32

// Camera pin defines
#define CAM_PIN_PWDN -1  // Power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 21   // External clock to camera
#define CAM_PIN_SIOD 26   // I2C data (SDA) to sensor
#define CAM_PIN_SIOC 27   // I2C clock (SCL) to sensor
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25  // Frame sync
#define CAM_PIN_HREF 23   // Line sync
#define CAM_PIN_PCLK 22   // Pixel clock

// Maximum Theoretical ADC values of the different buttons in the voltage divider circuit
// In reality, the values were smaller by about 120 for the up, down, and select, on average
// Using the higher theoretical values will allow for small fluctuations in the ADC readings
#define NONE_ADC 0          // 0 (pull down resistor)
#define UP_ADC 1015         // 330/(330+1000) * 4095
#define DOWN_ADC 2048       // 330/(330+330) * 4095
#define SELECT_ADC 3141     // 330/(330+100) * 4095
#define BACK_ADC 4095       // 330/330 * 4095

// Queue sizes
#define BUTTON_QUEUE_SIZE 3          // small to prevent buffering
#define FRAME_DISPLAY_QUEUE_SIZE 1    // Experiment with this
#define FRAME_SAVE_QUEUE_SIZE 1       // Experiment with this
#define SYS_INFO_QUEUE_SIZE 1         // will change for graph mode
#define FILE_DELETE_QUEUE_SIZE 5      // Buffer a few delete requests
#define WIFI_QUEUE_SIZE 1
#define NTP_QUEUE_SIZE 1

// Other
#define DEBOUNCE_INTERVAL 100         // How often to check for button presses (change as needed)
#define NUM_SYS_DATA_POINTS 60        // How many seconds to record system data
#define MAX_WIFI 8                    // How many nearby wifi signals to display
#define MAX_FILENAME_LENGTH 32
#define MAIN_MENU_SIZE 5
#define GAMES_MENU_SIZE 3

#define NUM_BRICKS 8


// ============================= Enums =============================
enum ButtonState {
  NONE,
  UP,
  DOWN,
  SELECT,
  BACK
};

enum DisplayState {
  BOOT,
  MENU,
  CAMERA_FEED,
  SD_CARD,
  SYSTEM_DATA,
  WIFI,
  GAMES,
  GAME1,
  GAME2,
  GAME3
};


// ============================= Structs =============================
struct MenuItem {
  String label;
  DisplayState next_state;
};

struct SystemInfo {
  int min_free_heap;
  int cpu_freq;
  unsigned long uptime;
};

struct WiFiInfo {
  char ssid[32];
  char ip[20];
  int rssi;
  char mac[20];
  char nearby[MAX_WIFI][33];
  int nearbyCount;
};


// ============================= Camera Config =============================
static camera_config_t camera_config = {  
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,   // Good balence between quality and memory usage
    .frame_size = FRAMESIZE_240X240,    // 240x240 square for now, leaves room for button options below

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 5,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};


// ============================= Global Variables =============================

// Menu items
extern MenuItem main_menu[MAIN_MENU_SIZE];
extern MenuItem games_menu[GAMES_MENU_SIZE];

// State variables
extern DisplayState display_state;
extern DisplayState prev_state;
extern ButtonState button_state;
extern int button_index;
extern char game_input;
extern int last_button_index;

// Camera flags
extern bool camera_init;
extern bool save_next_frame;

// Menu flags
extern bool menu_init;
extern bool image_view;
extern bool image_shown;

// File system
extern int file_index;
extern int prev_file_index;
extern std::vector<String> filenames;
extern int num_files;

// System data
extern std::queue<int> heap_usage;
extern int min_heap;
extern int max_heap;

// Queue handles
extern QueueHandle_t frame_display_queue;
extern QueueHandle_t frame_save_queue;
extern QueueHandle_t button_queue;
extern QueueHandle_t sys_info_queue;
extern QueueHandle_t file_delete_queue;
extern QueueHandle_t wifi_queue;

// Task handles
extern TaskHandle_t frameCaptureTask_handle;
extern TaskHandle_t saveFrameToSDTask_handle;
extern TaskHandle_t deleteFromSDTask_handle;

// FPS tracking
extern int frames;
extern float prev_fps;
extern float fps;
extern unsigned long now;
extern unsigned long before;

// WiFi credentials
extern const char *SSID;
extern const char *PASSWORD;

// NTP info
extern const char* ntpServer;       // Server where we request the time
extern const long gmt_offset_sec;      // 8 hours for Toronto time
extern const int daylight_offset_sec;  // DST offset
extern tm t;                           // Holds current data and time from NTP server

// TFT Display
extern TFT_eSPI tft;
