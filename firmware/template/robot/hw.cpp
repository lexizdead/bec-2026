// hw.cpp — реалізація «заліза». Готово й перевірено на кабель-укладачі.
#include "hw.h"
#include <Servo.h>
#include <EEPROM.h>

// ======================= мотори =======================
static void side(int pwm, uint8_t en, uint8_t a, uint8_t b, bool inv) {
  if (inv) pwm = -pwm;
  pwm = constrain(pwm, -PWM_MAX, PWM_MAX);
  digitalWrite(a, pwm > 0);
  digitalWrite(b, pwm < 0);
  analogWrite(en, abs(pwm));
}
void motorsBegin() {
  uint8_t p[] = {PIN_ENA, PIN_IN1, PIN_IN2, PIN_ENB, PIN_IN3, PIN_IN4};
  for (uint8_t i = 0; i < 6; i++) pinMode(p[i], OUTPUT);
}
void motorsSet(int l, int r) {
  side(l, PIN_ENA, PIN_IN1, PIN_IN2, MOTOR_L_INVERT);
  side(r, PIN_ENB, PIN_IN3, PIN_IN4, MOTOR_R_INVERT);
}
void motorsBrake() {
  digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, HIGH);
  digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, HIGH);
  analogWrite(PIN_ENA, 255); analogWrite(PIN_ENB, 255);
}
void motorsCoast() { analogWrite(PIN_ENA, 0); analogWrite(PIN_ENB, 0); }

// ======================= датчики =======================
int sRaw[N_SENSORS], sVal[N_SENSORS];

void sensorsBegin() {}
void sensorsRead() {
  for (uint8_t i = 0; i < N_SENSORS; i++) {
    sRaw[i] = analogRead(SENSOR_PINS[i]);
    long span = S.black[i] - S.white[i];
    if (span == 0) span = 1;
    sVal[i] = constrain((long)(sRaw[i] - S.white[i]) * 1000L / span, 0, 1000);
  }
}
// Водіть датчиками над білим і чорним — запам'ятовуємо мін/макс.
bool sensorsCalibrate(unsigned long ms) {
  int lo[N_SENSORS], hi[N_SENSORS];
  for (uint8_t i = 0; i < N_SENSORS; i++) { lo[i] = 1023; hi[i] = 0; }
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    for (uint8_t i = 0; i < N_SENSORS; i++) {
      int v = analogRead(SENSOR_PINS[i]);
      lo[i] = min(lo[i], v); hi[i] = max(hi[i], v);
    }
    digitalWrite(PIN_LED, (millis() / 100) & 1);
  }
  digitalWrite(PIN_LED, LOW);
  bool ok = true;
  for (uint8_t i = 0; i < N_SENSORS; i++) {
    Serial.print(F("sensor ")); Serial.print(i);
    Serial.print(F(": white ")); Serial.print(lo[i]);
    Serial.print(F(" black ")); Serial.println(hi[i]);
    if (hi[i] - lo[i] < 100) { ok = false; Serial.println(F("  ! замалий розкид — датчик не бачив чорного?")); continue; }
    // TCRT5000 AO: зазвичай чорне = більше значення. Якщо навпаки — поміняються місцями і все одно працює.
    S.white[i] = lo[i]; S.black[i] = hi[i];
  }
  return ok;
}

// ======================= енкодери =======================
static volatile long ticks[2] = {0, 0};
static void isrL() { ticks[0]++; }
static void isrR() { ticks[1]++; }
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
long odomTicks(uint8_t s) { noInterrupts(); long t = ticks[s & 1]; interrupts(); return t; }
void odomReset() { noInterrupts(); ticks[0] = ticks[1] = 0; interrupts(); }
float odomMM() {
#if ENC_COUNT >= 2
  return (odomTicks(0) + odomTicks(1)) * 0.5f * MM_PER_TICK;
#elif ENC_COUNT == 1
  return odomTicks(0) * MM_PER_TICK;
#else
  return 0;
#endif
}

// ======================= серво =======================
static Servo srv;
static int srvDeg = SERVO_HOME_DEG;
void servoBegin() { srv.attach(PIN_SERVO, 500, 2400); srv.write(srvDeg); }
void servoGo(int deg) {
  deg = constrain(deg, 0, 180);
  while (srvDeg != deg) { srvDeg += (deg > srvDeg) ? 1 : -1; srv.write(srvDeg); delay(6); }
}

// ======================= налаштування =======================
Settings S;
void settingsDefaults() {
  S.version = SETTINGS_VERSION;
  S.kp = DEF_KP; S.ki = DEF_KI; S.kd = DEF_KD;
  S.basePwm = DEF_BASE_PWM;
  for (uint8_t i = 0; i < N_SENSORS; i++) { S.white[i] = 100; S.black[i] = 900; }
  for (uint8_t i = 0; i < 4; i++) S.user[i] = 0;
}
void settingsLoad() {
  EEPROM.get(0, S);
  if (S.version != SETTINGS_VERSION) { settingsDefaults(); Serial.println(F("EEPROM: дефолти")); }
}
void settingsSave() { EEPROM.put(0, S); }
void settingsPrint() {
  Serial.print(F("P")); Serial.print(S.kp, 3);
  Serial.print(F(" I")); Serial.print(S.ki, 3);
  Serial.print(F(" D")); Serial.print(S.kd, 3);
  Serial.print(F(" B")); Serial.print(S.basePwm);
  for (uint8_t i = 0; i < 4; i++) { Serial.print(F(" U")); Serial.print(i); Serial.print('='); Serial.print(S.user[i]); }
  Serial.println();
}
