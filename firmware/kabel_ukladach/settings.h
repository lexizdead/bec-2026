#pragma once
#include <Arduino.h>

// Те, що калібрується або підбирається на трасі. Зберігається в EEPROM.
struct Settings {
  uint16_t magic;
  int16_t lineWhite, lineBlack;   // сирі значення TCRT лінії
  int16_t markWhite, markBlack;   // сирі значення TCRT міток
  float kp, ki, kd;
  int16_t basePwm, slowPwm, approachPwm;
  int16_t dropOffsetMm;           // від датчика міток до точки падіння шканта
  int16_t dropDelayMs;            // те саме в часі (без енкодерів)
};

extern Settings S;

void settingsLoad();      // з EEPROM або дефолти з config.h
void settingsSave();
void settingsDefaults();
void settingsPrint();
