/*
Holds all the FreeRTOS tasks
*/

#pragma once
#include "globals.h"


void buttonTask(void* parameter);

void frameCaptureTask(void* parameter);

void systemDataTask(void* parameter);

void saveFrameToSDTask(void* parameter);

void deleteFromSDTask(void* parameter);

void wifiDataTask(void *parameter);

void ntpTimeTask(void *parameter);

void displayTask(void* parameter);
