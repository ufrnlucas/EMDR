// Arquivo: motor_control.h

#pragma once
#include "emdr_config.h"

void setup_motor_led();
void setupDRV(Adafruit_DRV2605& drv, uint8_t addr);
void setMotorIntensity(Adafruit_DRV2605& drv, uint8_t intensity);