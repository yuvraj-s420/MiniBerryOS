/*

Helper functions

*/

#pragma once
#include <esp_camera.h>


void loadFileNames();

void printFileNames();

void generateXYArrays(int x_c[], int y_c[]);    // Generates arrays to be passed to drawGraph()

void initHeapQueue();                           // Sets all values in heap_queue to min

void initWiFi();

void initNTP();

void initSD();

void initTFT();

void initQueues();

bool checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

void transposeImage(camera_fb_t *fb, int bytes_per_pixel);

void transposeImageInPlace(camera_fb_t *fb);
