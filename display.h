/*

Various drawing functions for the TFT display

*/

#pragma once
#include "globals.h"

void drawBoot();

void drawMenu(MenuItem *menu, int num_items);

void updateMenuHighlight(MenuItem *menu, int num_items);

void drawStatusBar();

void drawSystemData();

void drawWifi();

void drawCameraFeed();

void drawCameraButton();

void drawFiles();

void drawImageViewer();

void drawGraph(int x_c[], int y_c[], int len, int y_min, int y_max, int x, int y, int w, int h, char *x_label, char *y_label, char *title, uint32_t color);

void initGame1(TFT_eSprite *paddle, TFT_eSprite *ball, TFT_eSprite bricks[]);

void updateGame1(TFT_eSprite* paddle, TFT_eSprite* ball, TFT_eSprite bricks[], char res);


void drawGame2();

void drawGame3();
