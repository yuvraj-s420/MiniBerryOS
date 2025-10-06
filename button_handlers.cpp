#include "button_handlers.h"


void handleButtonMenu(MenuItem* menu, int n_buttons){
  // Recieves button state from queue and updates button_index accordingly
  // Increments or deincrements based on the menu and number of buttons passed

  button_state = NONE;
  if(!xQueueReceive(button_queue, &button_state, 0)){
    //Serial.println("Button State failed to be recieved from queue! ");
    return;
  }

  if (button_state == DOWN){    // Increment button_index but clamp to 0 if exceeds n_buttons (wrap to top)
    button_index = (++button_index > (n_buttons - 1)) ? 0 : button_index;   
  }
  else if (button_state == UP){ // Deincrement button_index but clamp to last button (wrap to bottom)
    button_index = (--button_index < 0) ? n_buttons - 1 : button_index;  
  }
  else if (button_state == SELECT){
    prev_state = display_state;                       // Save previous state
    display_state = menu[button_index].next_state;    // Get next state of the selected button
    button_index = 0;                                 // reset button index
    menu_init = false;                                // Reset menu init flag
  }
  else if (button_state == BACK){

    display_state = prev_state;                       // Go to previous state
    button_index = 0;                                 // Reset button index
    menu_init = false;
  }   
}

void handleButtonSimple(){
  // Handles buttons for simple display only states like WIFI and SYSTEM_INFO

  button_state = NONE;
  if(!xQueueReceive(button_queue, &button_state, 0)){
    //Serial.println("Button State failed to be recieved from queue! ");
    return;
  }

  if (button_state == BACK){
    display_state = MENU;
    menu_init = false;
    button_index = 0;
  }

}

void handleButtonCamera(){

  button_state = NONE;
  if(!xQueueReceive(button_queue, &button_state, 0)){
    //Serial.println("Button State failed to be recieved from queue! ");
    return;
  }

  if (button_state == SELECT){          // Save frame flag
    save_next_frame = true;
    Serial.println("Will save next frame");
  }
  else if (button_state == BACK){       // Return to menu
    display_state = MENU;

    vTaskDelete(frameCaptureTask_handle);           // Delete tasks
    frameCaptureTask_handle = NULL;
    vTaskDelete(saveFrameToSDTask_handle);
    saveFrameToSDTask_handle = NULL;

    camera_fb_t *fb;
    while (xQueueReceive(frame_display_queue, &fb, 0) == pdTRUE && xQueueReceive(frame_save_queue, &fb, 0) == pdTRUE){
      // Pass through and drain every item in the queue
    }

    esp_camera_return_all();
    esp_camera_deinit();                            // Deinit camera drivers to free up PSRAM
    camera_init = false;                            // Reset camera init flag
    menu_init = false;                              // Reset menu init flag
    save_next_frame = false;                        // reset SD save flag
    button_index = 0;
  }
}

char handleButtonGame(){
  // Handles button inputs for the games (up, down, select) and back sends to game menu

  button_state = NONE;
  game_input = 'N';
  if(!xQueueReceive(button_queue, &button_state, 0)){
    //Serial.println("Button State failed to be recieved from queue! ");
  }

  if (button_state == UP){
    game_input = 'U';
  }
  else if (button_state == DOWN){
    game_input = 'D';
  }
  else if (button_state == SELECT){
    game_input = 'S';
  }
  else if (button_state == BACK){
    display_state = GAMES;
    prev_state = MENU;
    menu_init = false;
    game_input = 'B';               // Exit
    button_index = 0;
  }
  return game_input;
}

void handleButtonFiles(){
  // Handles button presses for the file viewing system

  button_state = NONE;
  if (!xQueueReceive(button_queue, &button_state, 0)){
    //Serial.println("Button State failed to be recieved from queue! ");
    return;
  }

  if (!image_view){
    if (button_state == DOWN){
      file_index++;
    }
    else if (button_state == UP){
      file_index--;
    }
    else if (button_state == SELECT){
      image_view = true;
    }
    else if (button_state == BACK){       // Return to menu
      vTaskDelete(deleteFromSDTask_handle);
      deleteFromSDTask_handle = NULL;
      display_state = MENU;
      prev_state = MENU;
      menu_init = false;
      image_view = false;
      button_index = 0;
      prev_file_index = -1;
    }

    if (file_index < 0){                    // Clamp to 0 
    file_index = 0;   
    }
    else if (file_index >= num_files){      // Clamp to index of last file
      file_index = num_files - 1;
    }

  } else{
    if (button_state == SELECT){                      // Delete file

      //Serial.println(filenames[file_index]);
      //String filename = filenames[file_index];
      //Serial.println(filename);

      // cannot use char* or strings in queues, must pass char arrays to prevent dangling pointers
      char filename[MAX_FILENAME_LENGTH] = {0};

      strncpy(filename, filenames[file_index].c_str(), MAX_FILENAME_LENGTH - 1);
      filename[MAX_FILENAME_LENGTH - 1] = '\0';

      xQueueSend(file_delete_queue, &filename, 0);    // Send filename to delete queue
      image_view = false;                             // Return to the file viewer state
      image_shown = false;
      prev_file_index = -1;                           // Allow redraw of file viewer
      tft.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, TFT_BLACK);
    }
    else if (button_state == BACK){       
      image_view = false;                             // Return to the file viewer state
      image_shown = false;
      menu_init = false;
      prev_file_index = -1;
    }
  }
}

