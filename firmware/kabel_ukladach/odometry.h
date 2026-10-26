#pragma once
#include <Arduino.h>

void odomBegin();
void odomReset();
uint32_t odomTicksL();
uint32_t odomTicksR();
float odomMM();          // пройдена відстань центру робота, мм (0 без енкодерів)
bool odomAvailable();    // чи є енкодери (ENC_COUNT > 0)
