#include "odometry.h"
#include "config.h"

// Енкодери без напрямку: робот їде лише вперед (PWM у follow не від'ємний).
static volatile uint32_t ticksL = 0, ticksR = 0;
static volatile unsigned long lastL = 0, lastR = 0;

static void isrL() {
  unsigned long t = micros();
  if (t - lastL >= ENC_DEBOUNCE_US) { ticksL++; lastL = t; }
}
static void isrR() {
  unsigned long t = micros();
  if (t - lastR >= ENC_DEBOUNCE_US) { ticksR++; lastR = t; }
}

void odomBegin() {
#if ENC_COUNT >= 1
  pinMode(PIN_ENC_L, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_L), isrL, CHANGE);
#endif
#if ENC_COUNT >= 2
  pinMode(PIN_ENC_R, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_R), isrR, CHANGE);
#endif
}

void odomReset() {
  noInterrupts(); ticksL = 0; ticksR = 0; interrupts();
}

uint32_t odomTicksL() { noInterrupts(); uint32_t v = ticksL; interrupts(); return v; }
uint32_t odomTicksR() { noInterrupts(); uint32_t v = ticksR; interrupts(); return v; }

bool odomAvailable() { return ENC_COUNT > 0; }

float odomMM() {
#if ENC_COUNT >= 2
  return (odomTicksL() + odomTicksR()) * 0.5f * MM_PER_TICK;
#elif ENC_COUNT == 1
  return odomTicksL() * MM_PER_TICK;
#else
  return 0;
#endif
}
