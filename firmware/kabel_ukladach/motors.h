#pragma once
#include <Arduino.h>

void motorsBegin();
// -255..255 для кожного колеса; від'ємне = назад
void motorsSet(int left, int right);
void motorsBrake();   // активне гальмо
void motorsCoast();   // вільний вибіг
