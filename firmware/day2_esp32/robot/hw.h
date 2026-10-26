// ======================================================================
//  hw.h — «залізо»: мотори, датчики, HC-SR04, зумер, серво, налаштування.
//  Однакове для будь-якого завдання — логіку дня пишемо в robot.ino.
// ======================================================================
#pragma once
#include "config.h"

// ---- мотори (L298N) ----
void motorsBegin();
void motorsSet(int left, int right);   // -255..255, мінус = назад
void motorsBrake();                    // активне гальмо (IN1 = IN2 = HIGH)
void motorsCoast();                    // вільний хід

// ---- датчики лінії ----
void sensorsBegin();
void sensorsRead();                    // оновлює масиви нижче
extern int sRaw[N_SENSORS];            // сирі 0..4095
extern int sVal[N_SENSORS];            // 0 (біле) .. 1000 (чорне)
bool sensorsCalibrate(unsigned long ms);
inline bool isBlack(uint8_t i) { return sVal[i] > ON_LINE; }

// ---- інше ----
float distCm();                        // HC-SR04, 999 = нічого
float battV();
void  buzz(bool on);
void  servoBegin();
void  servoWrite(int deg);

// ---- налаштування (Preferences у flash ESP32, переживають перезавантаження) ----
struct Settings {
  float kp, kd;
  int base, pivot, target, mmPerS;
  int bypass, bpSide, waitMs, tTurn, arcIn, arcOut, tArcMin, tArcMax, tAlign;
  int white[N_SENSORS], black[N_SENSORS];
};
extern Settings S;
void settingsLoad();
void settingsSave();
void settingsDefaults();
void settingsPrint();
