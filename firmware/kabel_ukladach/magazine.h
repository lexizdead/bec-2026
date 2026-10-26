#pragma once
#include <Arduino.h>

void magBegin();          // серво в позицію 0 (усі шканти на місці)
void magHome();
uint8_t magDropped();     // скільки вже скинуто
bool magEmpty();
void magDropNext();       // блокує ~0.5 с: поворот на наступну камеру + очікування падіння
