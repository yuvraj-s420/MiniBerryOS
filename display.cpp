#include <SD_MMC.h>
#include "display.h"
#include "globals.h"
#include "helpers.h"


void drawBoot(){

}

void drawStatusBar(){
  // Draws status bar with black background, battery level, cpu percentage, fps

  static int w = SCREEN_WIDTH, h = STATUS_BAR_HEIGHT;
  static int battery_x = 2, battery_y = 2;
  static int battery_w = 30, battery_h = 14;
  static int fps_text_x = SCREEN_WIDTH;

  static uint32_t text_color = TFT_WHITE;
  static uint32_t bg_color = TFT_BLACK;
  static uint32_t battery_outline = TFT_DARKGREY;
  static uint32_t battery_fill = TFT_GREEN;

  static int prev_min = -1;                         // Used to update screen only when time changes

  if((int)prev_fps != (int)fps){
    tft.fillRect(0, 0, w/3, h, bg_color);   // Clear first and last third to refresh values
    tft.fillRect(2*w/3, 0, w/3, h, bg_color);

    // Battery symbol with percentage
    tft.fillRect(battery_x, battery_y, battery_w, battery_h, battery_fill);
    tft.drawRect(battery_x, battery_y, battery_w, battery_h, battery_outline);
    tft.setTextColor(text_color, battery_fill);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("100%", battery_x + battery_w/2, battery_y + battery_h/2);       // Just for show

    // FPS data 
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(text_color, bg_color);
    tft.drawString("FPS: " + String((int)fps), fps_text_x, STATUS_BAR_HEIGHT / 2);
    tft.setTextDatum(MC_DATUM);

    tft.drawFastHLine(0, STATUS_BAR_HEIGHT-1, SCREEN_WIDTH, TFT_WHITE);
    prev_fps = fps;
  }

  // NTP time
  if (prev_min != t.tm_min){
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text_color, bg_color);
    tft.fillRect(w/3, 0, w/3, STATUS_BAR_HEIGHT, TFT_BLACK);       // Clear 2nd third of screen to refresh
    
    char time_str[6]; // "HH:MM" + null terminator
    sprintf(time_str, "%02d:%02d", t.tm_hour, t.tm_min);
    tft.drawString(time_str, SCREEN_WIDTH / 2, STATUS_BAR_HEIGHT / 2);    // Draw time
    prev_min = t.tm_min;

    tft.drawFastHLine(0, STATUS_BAR_HEIGHT-1, SCREEN_WIDTH, TFT_WHITE);
  }
}

void drawMenu(MenuItem *menu, int num_items){
  // General function for drawing simple menus
  // Draws rectangles for each menu item, and highlights outline depending on button_index

  int w = SCREEN_WIDTH, h = MENU_ITEM_HEIGHT;
  uint32_t bg_color = TFT_DARKGREY;
  uint32_t outline_color = TFT_BLUE;
  uint32_t highlight_color = TFT_WHITE;
  uint32_t text_color = TFT_WHITE;

  for (int i = 0; i < num_items; i++){

    tft.fillRect(0, STATUS_BAR_HEIGHT + h * i, w, h, bg_color);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, bg_color);              // background of text same as rectangle
    tft.setTextDatum(MC_DATUM);                         // middle-center alignment
    tft.drawString(menu[i].label, 120, STATUS_BAR_HEIGHT + h * i + h / 2);        // centered in the button
    tft.drawRect(0, STATUS_BAR_HEIGHT + h * i, w, h, outline_color);
    
  }

  tft.setTextSize(1);

  // Draw initial highlight
  tft.drawRect(0, STATUS_BAR_HEIGHT + button_index * h, w, h, highlight_color);

  last_button_index = button_index; // Track previous highlight
    
}

void updateMenuHighlight(MenuItem* menu, int num_items){
  // Only updates highlight of menu when button index changes

  uint32_t outline_color = TFT_BLUE;
  uint32_t highlight_color = TFT_WHITE;

  if (last_button_index != button_index) {
    int y_old = STATUS_BAR_HEIGHT + last_button_index * MENU_ITEM_HEIGHT;
    int y_new = STATUS_BAR_HEIGHT + button_index * MENU_ITEM_HEIGHT;

    // Remove highlight from old button
    tft.drawRect(0, y_old, SCREEN_WIDTH, MENU_ITEM_HEIGHT, outline_color);

    // Draw highlight on new button
    tft.drawRect(0, y_new, SCREEN_WIDTH, MENU_ITEM_HEIGHT, highlight_color);

    last_button_index = button_index; // Update tracker
  }
}

void drawSystemData() {
  // Draws all the system data from the queue (live information)
  // Will update to instead show a graph of useful stats like:
  // Graph of PSRAM, Heap, CPU% data over time

  SystemInfo info;

  if (!xQueueReceive(sys_info_queue, &info, 0)) {
    return;
  }
    
  tft.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int x = 10;
  int y = STATUS_BAR_HEIGHT + 10;
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("System Data", SCREEN_WIDTH/2, y); y += 20;
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("Min Heap: " + String(info.min_free_heap) + " B", x, y); y += 20;
  tft.drawString("CPU Freq: " + String(info.cpu_freq) + " MHz", x, y); y += 20;
  tft.drawString("Uptime: " + String(info.uptime) + " s", x, y); y += 20;

  const int graph_width = 220;
  const int graph_height = 200;
  const uint32_t graph_color = TFT_BLUE;

  // Create buffers for x and y points
  int x_c[NUM_SYS_DATA_POINTS] = {0};
  int y_c[NUM_SYS_DATA_POINTS] = {0};
  generateXYArrays(x_c, y_c);
  drawGraph(x_c, y_c, NUM_SYS_DATA_POINTS, min_heap, max_heap, x, y,
            graph_width, graph_height, "Time (s)", "kB", "Heap Usage Within Last Minute", graph_color);
}

void drawWifi(){

  static int sectionHeight = 20;  
  int y = STATUS_BAR_HEIGHT + sectionHeight / 2;

  const uint32_t bg_color = TFT_BLACK;
  const uint32_t text_color = TFT_WHITE;

  WiFiInfo info;  

  if (!xQueueReceive(wifi_queue, &info, 0)){
    return;
  } else{
    tft.fillRect(0,STATUS_BAR_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);

    // Main WiFi info section
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(text_color, bg_color);
    tft.drawString("WiFi Info", SCREEN_WIDTH / 2, y);
    y += sectionHeight;

    tft.setTextDatum(ML_DATUM);
    tft.drawString(String("SSID: ") + String(info.ssid), 0, y);
    y += sectionHeight;
    tft.drawString(String("IP: ")   + String(info.ip),   0, y);
    y += sectionHeight;
    tft.drawString("RSSI: " + String(info.rssi) + " dBm", 0, y);
    y += sectionHeight;
    tft.drawString(String("MAC: ")  + String(info.mac),  0, y);
    y += sectionHeight + 10;

    // Divider line
    tft.drawLine(0, y, SCREEN_WIDTH, y, TFT_WHITE);
    y += 10;

    // Scan nearby networks
    int n = info.nearbyCount;
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Nearby WiFi", SCREEN_WIDTH / 2, y);
    y += sectionHeight;

    tft.setTextDatum(ML_DATUM);
    if (n == 0) {
      tft.drawString("No networks found", 0, y);
    } else {
      for (int i = 0; i < n && i < MAX_WIFI; i++) { 
        tft.drawString(info.nearby[i], 0, y);
        y += sectionHeight;
      }
    }
    tft.setTextDatum(MC_DATUM);
  }

  
}

void drawCameraFeed(){

  camera_fb_t *fb;
  if (!xQueueReceive(frame_display_queue, &fb, 0)){
    //Serial.println("Camera frame was not recieved from the queue!");
    return;
  }


  // Push image to the screen
  tft.pushImage(0, STATUS_BAR_HEIGHT, fb->width, fb->height, (uint16_t *)fb->buf);

  if (save_next_frame){
    tft.fillRect(0, STATUS_BAR_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT, TFT_WHITE);     // Flash white screen before taking photo (visual signal of photo being taken)

  }
  else{
    esp_camera_fb_return(fb);       // Return the fb if a save was not requested
  }
}

void drawCameraButton(){
  // Draw option to save
  static int option_w = SCREEN_WIDTH, option_h = SCREEN_HEIGHT - (STATUS_BAR_HEIGHT + IMAGE_HEIGHT);
  static uint32_t bg_color = TFT_BLUE;
  static uint32_t outline_color = TFT_BLUE;
  static uint32_t highlight_color = TFT_WHITE;
  static uint32_t text_color = TFT_WHITE;

  tft.fillRect(0, STATUS_BAR_HEIGHT + 240, option_w, option_h, bg_color);
  tft.drawRect(0, STATUS_BAR_HEIGHT + 240, option_w, option_h, highlight_color);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(text_color, bg_color);
  tft.drawString("Save to SD card", option_w / 2, STATUS_BAR_HEIGHT + 240 + option_h/2);
}

void drawFiles(){

  static int max_files = 10;                               // 10 files shown at a time
  static int start = 0, end = max_files-1;                 // Indexes for file boxes

  static int w = SCREEN_WIDTH, h = (SCREEN_HEIGHT - STATUS_BAR_HEIGHT) / max_files;  
  static uint32_t bg_color = TFT_DARKGREY;
  static uint32_t outline_color = TFT_BLUE;
  static uint32_t highlight_color = TFT_WHITE;
  static uint32_t text_color = TFT_WHITE;

  if (file_index != prev_file_index){         // Only update if something changed

    if (file_index > end){                    // Increment start and end index for viewport
      start++;
      end++;
    }
    else if (file_index < start){          // De-increment start and end index for viewport
      start--;
      end--;
    }

    if (start < 0){                         // Clamp start to 0
      start = 0;
    }

    if (end > num_files){                   // Clamp end to index of last file + 1
      end = num_files-1;
    }

    if (end != start + max_files - 1){      // Temporary fix to the end variable shrinking unexpectedly when files are deleted in certain indexes
                                            // I think the error is due to num_files changing after deletion but i dont know how to fix it
      end = start + max_files - 1;
    }

    for (int i = start; i <= end; i++){

      //Serial.printf("Start: %i\ti: %i\tfile_index: %i\tEnd: %i\n", start, i, file_index, end);

      String filename = filenames[i];

      tft.fillRect(0, STATUS_BAR_HEIGHT + h * (i - start), w, h, bg_color);
      tft.setTextColor(text_color, bg_color);                                            // background of text same as rectangle
      tft.setTextDatum(MC_DATUM);                                                       // middle-center alignment
      tft.drawString(String(i) + ":  " + filename, 120, STATUS_BAR_HEIGHT + h * (i - start) + h / 2);    

      if (file_index == i){
        tft.drawRect(0, STATUS_BAR_HEIGHT + h * (i - start), w, h, highlight_color);    // Draw outline highlighted
      }
      else{
        tft.drawRect(0, STATUS_BAR_HEIGHT + h * (i - start), w, h, outline_color);      // Draw outline
      }
    }
    prev_file_index = file_index;
  }
}

void drawImageViewer(){
  //

  if (!image_shown){
  Serial.printf("Opening file at index: %i\tWith name: %s\n", file_index, filenames[file_index].c_str());
  
  fs::File file = SD_MMC.open(filenames[file_index], FILE_READ);

  // Allocate around 115kB for the RGB565 240x240 image (alternative is to go row by row to reduce ram usage)
  uint16_t *imgbuf = new uint16_t[IMAGE_WIDTH * IMAGE_HEIGHT];   
  file.read((uint8_t *)imgbuf ,IMAGE_WIDTH * IMAGE_HEIGHT * 2);
  file.close();

  tft.pushImage(0, STATUS_BAR_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT, imgbuf);

  delete[] imgbuf;                                               // Delete imgbuf from memory

  // Draw option to delete from SD
  static int option_w = SCREEN_WIDTH, option_h = SCREEN_HEIGHT - IMAGE_HEIGHT - STATUS_BAR_HEIGHT;
  static uint32_t bg_color = TFT_RED;
  static uint32_t outline_color = TFT_BLUE;
  static uint32_t highlight_color = TFT_WHITE;
  static uint32_t text_color = TFT_WHITE;

  tft.fillRect(0, STATUS_BAR_HEIGHT + IMAGE_HEIGHT, option_w, option_h, bg_color);
  tft.drawRect(0, STATUS_BAR_HEIGHT + IMAGE_HEIGHT, option_w, option_h, highlight_color);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(text_color, bg_color);
  tft.drawString("DELETE FROM SD CARD", option_w / 2, STATUS_BAR_HEIGHT + IMAGE_HEIGHT + option_h/2);

  image_shown = true;
  }
}

void drawGraph(int x_c[], int y_c[], int len, int y_min, int y_max, int x, int y, int w, int h, char* x_label, char* y_label, char* title, uint32_t color){
  // Draws a graph on screen given start x, y, width, height, points, labels, line color

  // ==================== Constants ====================
  const uint32_t bg_color = TFT_BLACK;
  const uint32_t graph_color = TFT_WHITE;
  const uint32_t text_color = TFT_WHITE;
  const uint32_t line_color = color;  

  const int graph_offset = 20;                    // Offset from outline and x,y axis lines
  const int x_axis_start = x + graph_offset;
  const int x_axis_end = x + w - graph_offset;
  const int y_axis_start = y + graph_offset / 2;
  const int y_axis_end = y + h - graph_offset;

  const int num_x_ticks = 10;
  const int num_y_ticks = 4;

  // ==================== General Structure ====================
  tft.fillRect(x, y, w, h, bg_color);
  tft.drawRect(x, y, w, h, graph_color);          // Outline of graph
  tft.drawLine(x_axis_start, y_axis_start, x_axis_start, y_axis_end, graph_color);  // x and y axis lines
  tft.drawLine(x_axis_start, y_axis_end, x_axis_end, y_axis_end, graph_color);

  tft.setTextColor(text_color, bg_color);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(title, x + w / 2, y);            // Title in top middle of graph area
  tft.drawString(x_label, x + w / 2, y + h);      // x label in bottom middle of graph area
  tft.drawString(y_label, x, y + h / 2);          // y label in left center of graph area

  // ==================== x tick values ====================
  // Assumes that input x are increasing values ex: 1,2,3,4,5,6....
  const int num_pixels_x_axis = x_axis_end - x_axis_start;           // Number of pixels in x-axis
  const int spacing_x = num_pixels_x_axis / (num_x_ticks - 1);

  for (int i = 1; i <= num_x_ticks; i++){

    int index = (i - 1) * len / num_x_ticks;
    int _x = x_axis_start + (i - 1) * spacing_x;
    int _y = y_axis_end + graph_offset / 3;
    tft.drawNumber(x_c[index], _x, _y);         
    //Serial.printf("Index: %i\tNum: %i\tx: %i\ty: %i\n", index, x_c[index], _x, _y);
  }

  // ==================== y tick values ====================
  const int num_pixels_y_axis = y_axis_end - y_axis_start;                  // Number of pixels in y axis
  const int spacing_y = num_pixels_y_axis / (num_y_ticks - 1);
  const int y_coord_range = y_max - y_min;                          // Range of y coordinate points
  const int tick_val_spacing = y_coord_range / (num_y_ticks - 1);

  for (int i = 1; i <= num_y_ticks; i++){

    int _x = x_axis_start - graph_offset / 3;
    int _y = y_axis_end - (i - 1) * spacing_y;
    int tick_val = y_min + (i - 1) * tick_val_spacing;
    tft.drawNumber(tick_val, _x, _y);
    //Serial.printf("Tick value: %i\tx: %i\ty: %i\n", tick_val, _x, _y);
  }

  // ==================== Point mapping ====================
  const int x_point_spacing = num_pixels_x_axis / len;       // How far apart in pixels each point will be

  // Start at first one
  int prev_x = x_axis_start;
  int prev_y = y_axis_end - ((y_c[0] - y_min) / (float)y_coord_range * num_pixels_y_axis);

  for (int i = 1; i < len; i++){

    int _x = x_axis_start + i * x_point_spacing;
    int _y = y_axis_end - ((y_c[i] - y_min) / (float)y_coord_range * num_pixels_y_axis);

    tft.drawLine(prev_x, prev_y, _x, _y, line_color);     // Draw line from prev point to current
    //Serial.printf("x: %i\ty: %i\t\n", _x, _y);

    prev_x = _x;
    prev_y = _y;
  }
}

void initGame1(TFT_eSprite* paddle, TFT_eSprite* ball, TFT_eSprite bricks[]){
  // Brick breaker game
  // initializes sprites for paddle, ball, and bricks
  
  const uint32_t paddle_color = TFT_WHITE;
  const uint32_t ball_color = TFT_RED;
  const uint32_t brick_color = TFT_BROWN;
  const uint32_t brick_outline = TFT_BLACK;

  const int paddle_h = 60;
  const int paddle_w = 10;
  const int ball_r = 7;
  const int brick_h = 40;
  const int brick_w = 20;

  static int paddle_x = 0;
  static int paddle_y = STATUS_BAR_HEIGHT + (SCREEN_HEIGHT - STATUS_BAR_HEIGHT) / 2 - paddle_h / 2;

  static int ball_x = paddle_x + paddle_w;
  static int ball_y = paddle_y + paddle_h / 2 - ball_r;

  // Delete previous sprites 
  paddle->deleteSprite();
  ball->deleteSprite();

  // Paddle is a rectangle starting in middle of screen
  paddle->createSprite(paddle_w, paddle_h);
  paddle->fillRect(0, 0, paddle_w, paddle_h, paddle_color);
  paddle->pushSprite(paddle_x, paddle_y);

  // Ball is beside and on middle of paddle
  ball->createSprite(ball_r * 2, ball_r * 2);
  ball->fillCircle(ball_r, ball_r, ball_r, ball_color);
  ball->pushSprite(ball_x, ball_y);
}

void updateGame1(TFT_eSprite* paddle, TFT_eSprite* ball, TFT_eSprite bricks[], char res){

  const int paddle_speed = 10;   // pixels per button input (tied to debounce interval)
  const int ball_speed = 1;      // 1 diagonal pixel per frame (tied to framerate, could tie to delta time)

  static bool x_rev = false;      // Ball to reverse movement
  static bool y_rev = false;      // Ball to reverse movement
  static bool paddle_hit = false;
  static bool lose_flag = false;

  const int paddle_h = 60;
  const int paddle_w = 10;
  const int ball_r = 7;
  const int brick_h = 30;
  const int brick_w = 15;

  static int paddle_x = 0;
  static int paddle_y = STATUS_BAR_HEIGHT + (SCREEN_HEIGHT - STATUS_BAR_HEIGHT) / 2 - paddle_h / 2;

  // Top left of ball
  static int ball_x = paddle_x + paddle_w;
  static int ball_y = paddle_y + paddle_h / 2 - ball_r;

  // When exiting, reset static variables 
  if (res == 'B'){         
    paddle_hit = false;
    lose_flag = false;

    paddle_x = 0;
    paddle_y = STATUS_BAR_HEIGHT + (SCREEN_HEIGHT - STATUS_BAR_HEIGHT) / 2 - paddle_h / 2;

    ball_x = paddle_x + paddle_w;
    ball_y = paddle_y + paddle_h / 2 - ball_r;
  }

  // Move paddle
  switch (game_input){
    case 'U':

      // Clear paddle area when moving
      tft.fillRect(paddle_x, paddle_y, paddle_w, paddle_h, TFT_BLACK);
      paddle_y -= paddle_speed;
      break;

    case 'D':

      // Clear paddle area when moving
      tft.fillRect(paddle_x, paddle_y, paddle_w, paddle_h, TFT_BLACK);      
      paddle_y += paddle_speed;
      break;

    case 'S':
      break;

    default:
      //
      break;
  }

  // Paddle clamping
  if (paddle_y <= STATUS_BAR_HEIGHT){
    paddle_y = STATUS_BAR_HEIGHT;
  } else if (paddle_y >= SCREEN_HEIGHT - paddle_h){
    paddle_y = SCREEN_HEIGHT - paddle_h;
  }



  tft.fillCircle(ball_x + ball_r, ball_y + ball_r, ball_r, TFT_BLACK);    // Clear previous position of ball

  // Move ball
  x_rev ? ball_x -= ball_speed : ball_x += ball_speed;      // Move ball left/ right
  y_rev ? ball_y -= ball_speed : ball_y += ball_speed;      // Move ball up/ down

  paddle_hit = checkCollision(paddle_x, paddle_y, paddle_w, paddle_h, ball_x, ball_y, ball_r * 2, ball_r * 2);

  // Ball clamping in x and direction reversal upon collision
  if (ball_x >= SCREEN_WIDTH - 2 * ball_r){     
    ball_x = SCREEN_WIDTH - 2 * ball_r;
    x_rev = !x_rev;
  } else if (paddle_hit){                     // Collision with paddle
    ball_x = paddle_w;
    x_rev = !x_rev;
  } else if (ball_x <= 0) {                   // Game Lost (infinite game for now during testing)
    ball_x = 0;
    x_rev = !x_rev;
    lose_flag = true;                         // Once ball leaves the frame, game is lost.
  }

  // Ball clamping in y and direction reversal upon hitting top and bottom
  if (ball_y <= STATUS_BAR_HEIGHT){
    ball_y = STATUS_BAR_HEIGHT;
    y_rev = !y_rev;
  } else if (ball_y >= SCREEN_HEIGHT - 2 * ball_r){
    ball_y = SCREEN_HEIGHT - 2 * ball_r;
    y_rev = !y_rev;
  }


  paddle->pushSprite(paddle_x, paddle_y);
  ball->pushSprite(ball_x, ball_y);
}

void drawGame2(){
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Game 2", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
}

void drawGame3(){
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Game 3", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
}
