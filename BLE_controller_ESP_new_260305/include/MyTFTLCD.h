#ifndef MYTFTLCD_H
#define MYTFTLCD_H

#include "main.h"

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>

void MyTFTLCD_Setup();
void MyTFTLCD_Loop();

extern TaskHandle_t myTFTLCDHandle;

extern bool BLE_Scan_Flag;

#endif