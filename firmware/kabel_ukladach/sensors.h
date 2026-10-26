#pragma once
#include <Arduino.h>

struct Reading {
  int lineRaw, markRaw;   // analogRead
  int line, mark;         // 0 = біле, 1000 = чорне
};

void sensorsBegin();
void sensorsRead(Reading &r);
// Калібрування: поставити датчики НАД БІЛИМ, запустити, потім кілька разів
// провести робота рукою через лінію і через мітку. Повертає true, якщо успішно.
bool sensorsCalibrate(unsigned long ms);
