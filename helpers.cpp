#include "helpers.h"
#include "globals.h"
#include <SD_MMC.h>
#include <WiFi.h>


void loadFileNames(){
  // Counts the number of files in the root directory of SD card and updates filenames array
  num_files = 0;                                        
  filenames.clear();                                      // Clear all elements from vector
  fs::File root = SD_MMC.open("/");                       // Root directory
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open root directory of SD");
    return;
  }

  Serial.println("Loading filenames...");

  fs::File file = root.openNextFile();

  while (file){
    
    // Using strings here to pushback in the vector seems to work
    String name = "/" + String(file.name());  // Make safe copy, prepend "/" here to remove need to do it later (when deleting or reopening)
    //Serial.println(name);
    if (name != "" && name != "/System Volume Information") {  // Skip empty names and the hidden folder
        filenames.push_back(name);      // Add to vector
        num_files++;                    
    }
    file.close();
    file = root.openNextFile();
  }
  file.close();
  root.close();
}

void printFileNames(){
  // Debugging
  for (int i = 0; i < filenames.size(); i++){
    Serial.println(filenames[i]);
  }
}

void generateXYArrays(int x_c[], int y_c[]){
  // Generates x and y arrays for use in drawGraph()

  std::queue<int> q = heap_usage;

  for (int i = 0; i < NUM_SYS_DATA_POINTS; i++){
    x_c[i] = i + 1;
    y_c[i] = q.front();
    q.pop();

    if (q.empty()){
      break;
    }
  }
}

void initHeapQueue(){
  // Initializes the heap queue to be full of minimum amount
  // This makes it so that the points on graph start at x axis and go up

  for (int i = 0; i < NUM_SYS_DATA_POINTS; i++){
    heap_usage.push(min_heap);
  }
}

void initWiFi(){
  // Connects to WiFi with credentials declared in globals.cpp

  WiFi.begin(SSID, PASSWORD);
  Serial.println("Connecting to WiFi with SSID: " + String(SSID));
  while (WiFi.status() != WL_CONNECTED){
      delay(1000);
      Serial.print(".");
  }
  Serial.println("Connected to " + String(SSID) + " with local IP address: ");
  Serial.println(WiFi.localIP());
}

void initNTP(){
  // Sets up NTP server for location based live time

  configTime(gmt_offset_sec, daylight_offset_sec, ntpServer);
  getLocalTime(&t);
  Serial.printf("Time at boot\t%i:%i", t.tm_hour, t.tm_min);
}

void initSD(){

  if (!SD_MMC.begin("/sdcard", true)) {            // Needs true for mode1bit ESP32 Wroover-E with SD pins 2,14,15
    Serial.println("Card Mount Failed");
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
  } else{
    Serial.println(cardType);
  }
}

void initTFT(){
  // Initialization for tft display
  tft.init();
  tft.setRotation(2);               // Orientation dependent
  tft.setSwapBytes(SWAP_BYTES);     // Correct for endianess
  tft.fillScreen(TFT_BLACK);
  Serial.println("TFT initialized");
}

void initQueues(){
  // Initialize all queues and check for error
  // Hangs if queues fail

  button_queue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(ButtonState));
  frame_display_queue = xQueueCreate(FRAME_DISPLAY_QUEUE_SIZE, sizeof(camera_fb_t *));
  frame_save_queue = xQueueCreate(FRAME_SAVE_QUEUE_SIZE, sizeof(camera_fb_t *));
  sys_info_queue = xQueueCreate(SYS_INFO_QUEUE_SIZE, sizeof(SystemInfo));
  file_delete_queue = xQueueCreate(FILE_DELETE_QUEUE_SIZE, MAX_FILENAME_LENGTH*sizeof(char));   // DO NOT PASS STRINGS IN QUEUE AS THEY PASS AROUND JUNK
  wifi_queue = xQueueCreate(WIFI_QUEUE_SIZE, sizeof(WiFiInfo));
  if (!button_queue || !frame_display_queue || !frame_save_queue || !sys_info_queue || !file_delete_queue || !wifi_queue) {
    Serial.println("Queue creation failed!");
    while(1) {}   // hang
  }
  Serial.println("Queues initialized");
}

bool checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2){
  // Given 2 rectangles, check wether there is a collision
  // For there to be a collision, the 

  bool res = true;

  int right_edge_1 = x1 + w1;
  int bottom_edge_1 = y1 + h1;
  int right_edge_2 = x2 + w2;
  int bottom_edge_2 = y2 + h2;

  // bottom_edge_2 below top_edge_1 AND top_edge_2 above bottom_edge_1
  res &= ((bottom_edge_2 >= y1) && (y2 <= bottom_edge_1));

  // right_edge_2 to the right of left_edge_1 AND left_edge_2 to the left of right_edge_1
  res &= ((right_edge_2 >= x1) && (x2 <= right_edge_1));

  return res;
}

void transposeImage(camera_fb_t *fb, int bytes_per_pixel){
  // Swap rows of x pixels and cols of y pixels
  // Only for images of equal width and height
  // bytes_per_pixel is 2 for RGB565

  const int size = fb->width;                         // Assume width and height are equal
  const int length = fb->len;                         // Bytes in fb

  // Allocate buffer for transposed img
  uint8_t *img_buf = new uint8_t[length];

  int counter = 0;
  int start_index = 0;
  int index = 0;

  for (int i = 0; i < length; i++){

    index = start_index + (counter * size) + (i % bytes_per_pixel);      // Index to copy from fb

    img_buf[i] = fb->buf[index];

    if ((i != 0) && (i % bytes_per_pixel == 0)){    // Accounts for multiple bytes per pixel 
      counter++;
    }

    if (counter == size){     // After each "row and col swap", reset counter and increment start_index
      counter = 0;
      start_index++;
    }
  }

  memcpy(fb->buf, img_buf, length);   // Copy transpose into fb
  delete[] img_buf;
}

void transposeImageInPlace(camera_fb_t *fb) {
  const int bpp = 2;
  const int width = fb->width;
  const int height = fb->height;

  if (width != height) {
      printf("Error: Image must be square to transpose.\n");
      return;
  }

  uint8_t *buf = fb->buf;

  for (int y = 0; y < height; y++) {
    for (int x = y + 1; x < width; x++) {
    int idx1 = (y * width + x) * bpp;
    int idx2 = (x * width + y) * bpp;

    // Swap 2 bytes (one pixel)
    uint8_t tmp0 = buf[idx1];
    uint8_t tmp1 = buf[idx1 + 1];
    buf[idx1]     = buf[idx2];
    buf[idx1 + 1] = buf[idx2 + 1];
    buf[idx2]     = tmp0;
    buf[idx2 + 1] = tmp1;
    }
  }
}
