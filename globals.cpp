#include "globals.h"


// Menu arrays
MenuItem main_menu[] = {                    // Main menu items
  {"System Data", SYSTEM_DATA},
  {"Wifi", WIFI},
  {"Camera Feed", CAMERA_FEED},
  {"SD Card", SD_CARD},
  {"Games", GAMES}
};

MenuItem games_menu[] = {                   // Games menu items
  {"Pong", GAME1},
  {"Game 2", GAME2},
  {"Game 3", GAME3}
};

// Tracking variables
DisplayState display_state = BOOT;  // Starting on the boot screen when powered on
DisplayState prev_state = MENU;     // Holds previous state for backtracking menus
ButtonState button_state = NONE;    // Holds button state
int button_index = 0;               // Keeps track of what menu item is currently highlighted
char game_input = 'N';              // Init to N for none
int last_button_index = 0;          
bool camera_init = false;           // Flag for whether camera is initialized
bool save_next_frame = false;       // Flag for whether the next frame should be saved
bool menu_init = false;             // Flag for initializing a menu
bool image_view = false;            // Flag for whether in file menu view or image view
bool image_shown = false;           // Flag to draw image viewer only once
int file_index = 0;                 // Index of current chosen file in SD root directory
int prev_file_index = -1;
std::vector<String> filenames(2);   // Contains all the filenames in the SD card (initialize to size 2)
int num_files = 0;
std::queue<int> heap_usage;         // Holds psram usage for graph
int min_heap = 100;                 // magic number, to a little lower than the lowest I have observed
int max_heap = 0;                   // Gets updated dynamically

// Queue handles for data
QueueHandle_t frame_display_queue = NULL;
QueueHandle_t frame_save_queue = NULL;
QueueHandle_t button_queue = NULL;
QueueHandle_t sys_info_queue = NULL;
QueueHandle_t file_delete_queue = NULL;
QueueHandle_t wifi_queue = NULL;

// Task Handles for suspending/ resuming tasks
TaskHandle_t frameCaptureTask_handle;
TaskHandle_t saveFrameToSDTask_handle;
TaskHandle_t deleteFromSDTask_handle;

// Used to determine fps
int frames = 0;
float prev_fps = 0;
float fps = 0;
unsigned long now = millis();
unsigned long before = 0;

// WiFi credentials
const char *SSID = "Mac House 🗣️";
const char *PASSWORD = "Sreyoisfat";

// NTP Server
const char* ntpServer = "pool.ntp.org";       // Server where we request the time
const long gmt_offset_sec = 8 * 60 * 60;      // 8 hours for Toronto time
const int daylight_offset_sec = 0 * 60 * 60;  // DST offset
tm t = {};                                    // Holds current data and time from NTP server

// TFT display
TFT_eSPI tft = TFT_eSPI();    