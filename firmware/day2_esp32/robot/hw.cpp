#include "hw.h"
#include <Preferences.h>

// ---------------- ШІМ: працює на ядрі ESP32 2.x і 3.x ----------------
#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void pwmInit(int pin, int freq, int bits) { ledcAttach(pin, freq, bits); }
static void pwmWrite(int pin, int duty) { ledcWrite(pin, duty); }
#else
static int chOf[40]; static int nextCh = 0;
static void pwmInit(int pin, int freq, int bits) { ledcSetup(nextCh, freq, bits); ledcAttachPin(pin, nextCh); chOf[pin] = nextCh++; }
static void pwmWrite(int pin, int duty) { ledcWrite(chOf[pin], duty); }
#endif

// ---------------- мотори ----------------
void motorsBegin() {
  int outs[] = {PIN_IN1, PIN_IN2, PIN_IN3, PIN_IN4};
  for (int p : outs) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
  pwmInit(PIN_ENA, PWM_FREQ, 8);
  pwmInit(PIN_ENB, PWM_FREQ, 8);
}
static void oneMotor(int en, int a, int b, int v) {
  v = constrain(v, -255, 255);
  digitalWrite(a, v > 0);
  digitalWrite(b, v < 0);
  pwmWrite(en, abs(v));
}
void motorsSet(int left, int right) {
  if (MOTOR_L_INVERT) left = -left;
  if (MOTOR_R_INVERT) right = -right;
  oneMotor(PIN_ENA, PIN_IN1, PIN_IN2, left);
  oneMotor(PIN_ENB, PIN_IN3, PIN_IN4, right);
}
void motorsBrake() {
  digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, HIGH);
  digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, HIGH);
  pwmWrite(PIN_ENA, 255); pwmWrite(PIN_ENB, 255);
}
void motorsCoast() { motorsSet(0, 0); }

// ---------------- датчики лінії ----------------
int sRaw[N_SENSORS], sVal[N_SENSORS];
void sensorsBegin() { analogReadResolution(12); }
void sensorsRead() {
  for (uint8_t i = 0; i < N_SENSORS; i++) {
    sRaw[i] = analogRead(SENSOR_PINS[i]);
    int span = S.black[i] - S.white[i];
    if (span == 0) span = 1;
    sVal[i] = constrain((long)(sRaw[i] - S.white[i]) * 1000 / span, 0, 1000);
  }
}
// Водіть роботом над білим і чорним CALIB_MS мс — запам'ятає min/max кожного датчика
bool sensorsCalibrate(unsigned long ms) {
  int lo[N_SENSORS], hi[N_SENSORS];
  for (uint8_t i = 0; i < N_SENSORS; i++) { lo[i] = 4095; hi[i] = 0; }
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    for (uint8_t i = 0; i < N_SENSORS; i++) {
      int v = analogRead(SENSOR_PINS[i]);
      lo[i] = min(lo[i], v); hi[i] = max(hi[i], v);
    }
    delay(2);
  }
  bool ok = true;
  for (uint8_t i = 0; i < N_SENSORS; i++) {
    Serial.printf("S%d: біле %d чорне %d\n", i, lo[i], hi[i]);
    if (hi[i] - lo[i] < 300) { ok = false; Serial.printf("  S%d: замала різниця — перевірте висоту/провід\n", i); continue; }
    S.white[i] = lo[i]; S.black[i] = hi[i];
  }
  return ok;
}

// ---------------- HC-SR04, АКБ, зумер, серво ----------------
float distCm() {
  if (digitalRead(PIN_ECHO) == HIGH) return 999;      // дешеві HC-SR04 «залипають» в 1 без відлуння — пропускаємо замір
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long t = pulseIn(PIN_ECHO, HIGH, 25000);   // до ~4 м
  return t ? t / 58.0f : 999;
}
float battV() { return analogReadMilliVolts(PIN_BATT) * 3 / 1000.0f; }

void buzz(bool on) {
  static bool was = false;
  if (on == was) return;
  was = on;
#if SIM
  if (on) tone(PIN_BUZZ, 2000); else noTone(PIN_BUZZ);
#else
  digitalWrite(PIN_BUZZ, on);
#endif
}

void servoBegin() { pwmInit(PIN_SERVO, 50, 16); servoWrite(SERVO_HOME); }
void servoWrite(int deg) {                       // 0.5–2.5 мс при 50 Гц
  long us = 500 + (long)constrain(deg, 0, 180) * 2000 / 180;
  pwmWrite(PIN_SERVO, us * 65535L / 20000L);
}

// ---------------- налаштування ----------------
Settings S;
static Preferences prefs;

void settingsDefaults() {
  S.kp = DEF_KP; S.kd = DEF_KD; S.base = DEF_BASE; S.pivot = DEF_PIVOT;
  S.target = DEF_TARGET; S.mmPerS = DEF_MM_S;
  S.bypass = DEF_BYPASS; S.bpSide = DEF_BP_SIDE; S.waitMs = DEF_WAIT_MS;
  S.tTurn = DEF_T_TURN; S.arcIn = DEF_ARC_IN; S.arcOut = DEF_ARC_OUT;
  S.tArcMin = DEF_T_ARC_MIN; S.tArcMax = DEF_T_ARC_MAX; S.tAlign = DEF_T_ALIGN;
  for (uint8_t i = 0; i < N_SENSORS; i++) { S.white[i] = 400; S.black[i] = 3200; }
}
void settingsLoad() {
  settingsDefaults();
  prefs.begin("sosny", true);
  if (prefs.getBytesLength("S") == sizeof(S)) prefs.getBytes("S", &S, sizeof(S));
  prefs.end();
}
void settingsSave() {
  prefs.begin("sosny", false);
  prefs.putBytes("S", &S, sizeof(S));
  prefs.end();
}
void settingsPrint() {
  Serial.printf("P%.1f D%.1f B%d V%d N%d  мм/с %d  об'їзд %d бік %d\n",
                S.kp, S.kd, S.base, S.pivot, S.target, S.mmPerS, S.bypass, S.bpSide);
  Serial.printf("об'їзд: чекати %d, відвернути %d, дуга %d/%d, %d..%d, доворот %d\n",
                S.waitMs, S.tTurn, S.arcIn, S.arcOut, S.tArcMin, S.tArcMax, S.tAlign);
  for (uint8_t i = 0; i < N_SENSORS; i++) Serial.printf("S%d біле %d чорне %d\n", i, S.white[i], S.black[i]);
}
