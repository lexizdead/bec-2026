// ======================================================================
//  hw.h — «залізо»: мотори, датчики, енкодери, серво, налаштування.
//  Ці функції однакові для будь-якого завдання — завтра їх майже не чіпаємо.
// ======================================================================
#pragma once
#include "config.h"

// ---- мотори (L298N) ----
void motorsBegin();
void motorsSet(int left, int right);   // -255..255, мінус = назад
void motorsBrake();                    // активне гальмо
void motorsCoast();                    // вільний хід

// ---- датчики ----
void sensorsBegin();
void sensorsRead();                    // оновлює масиви нижче
extern int sRaw[N_SENSORS];            // сирі 0..1023
extern int sVal[N_SENSORS];            // нормалізовані 0 (біле) .. 1000 (чорне)
bool sensorsCalibrate(unsigned long ms);
inline bool isBlack(uint8_t i) { return sVal[i] > BLACK_THR; }
inline bool isWhite(uint8_t i) { return sVal[i] < WHITE_THR; }

// ---- енкодери (якщо ENC_COUNT > 0) ----
void  odomBegin();
void  odomReset();
float odomMM();                        // пройдено мм від старту
long  odomTicks(uint8_t side);         // 0 = лівий, 1 = правий
inline bool odomAvailable() { return ENC_COUNT > 0; }

// ---- серво ----
void servoBegin();
void servoGo(int deg);                 // плавно до кута

// ---- налаштування в EEPROM ----
struct Settings {
  uint8_t version;
  float kp, ki, kd;
  int16_t basePwm;
  int16_t white[N_SENSORS], black[N_SENSORS];
  int16_t user[4];                     // вільні слоти під завдання дня (затримки, відстані…)
};
extern Settings S;
void settingsLoad();
void settingsSave();
void settingsDefaults();
void settingsPrint();
