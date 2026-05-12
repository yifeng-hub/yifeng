#ifndef CORELOOP_H
#define CORELOOP_H

#include "main.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void myTFTLCDLoop(void *pvParameters);

#endif