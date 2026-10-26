// ======================================================================
//  robot.ino — ЛОГІКА ЗАВДАННЯ (автомат станів). Шаблон команди sosny.
//
//  Як адаптувати під нове завдання (≈30 хв):
//    1. config.h   — піни, кількість датчиків, пороги.
//    2. enum State — назвіть кроки завдання (ЇХАТИ, ЗНАЙШЛИ_МІТКУ, СКИНУТИ, ...).
//    3. control()  — для кожного стану: що робимо і КОЛИ переходимо далі.
//    4. Serial-меню — тести залізу вже є; додайте команди під свій механізм.
//
//  Правило: у control() НІЯКИХ довгих delay() — цикл має крутитися кожні 4 мс.
//  Коротку дію (скинути предмет) можна робити з delay, бо робот у цей час стоїть.
// ======================================================================
#include "config.h"
#include "hw.h"

// ---------- 1. Стани ----------
enum State : uint8_t { ST_IDLE, ST_FOLLOW, ST_ACTION, ST_DONE };     // TODO: свої стани
static const char *const STATE_NAMES[] = {"IDLE", "FOLLOW", "ACTION", "DONE"};

static State st = ST_IDLE;
static unsigned long tCtl = 0, runStartMs = 0, stateStartMs = 0;
static uint8_t counter = 0;            // TODO: скільки разів зробили дію (шканти, кубики…)
static bool logRun = false;

static void go(State s) {              // перехід у новий стан + запис часу
  st = s; stateStartMs = millis();
  Serial.print(F("-> ")); Serial.println(STATE_NAMES[s]);
}
static unsigned long inState() { return millis() - stateStartMs; }

// ---------- 2. Будівельні блоки ----------
static float integ = 0, prevErr = 0;
static void pidReset() { integ = 0; prevErr = 0; }

// Рух по краю лінії одним датчиком. base — базова швидкість.
static void followLine(int base) {
  float err = sVal[S_LINE] - LINE_SETPOINT;
  integ = constrain(integ + err, -20000, 20000);
  float corr = S.kp * err + S.ki * integ + S.kd * (err - prevErr);
  prevErr = err;
  motorsSet(base - (int)corr, base + (int)corr);   // знак не той — поміняйте - і +
}

// Спрацювати ОДИН раз, коли умова тримається n циклів поспіль (антибрязкіт датчика)
static bool confirmed(bool cond, uint8_t &cnt, uint8_t n) {
  if (!cond) { cnt = 0; return false; }
  if (cnt < 255) cnt++;
  return cnt == n;
}

static bool buttonPressed() {
  static bool raw = HIGH, stable = HIGH;
  static unsigned long tChange = 0;
  bool v = digitalRead(PIN_BUTTON);
  if (v != raw) { raw = v; tChange = millis(); }
  if (millis() - tChange > 30 && v != stable) { stable = v; if (stable == LOW) return true; }
  return false;
}

static void finish(const __FlashStringHelper *why) {
  motorsBrake(); delay(150); motorsCoast();
  go(ST_DONE);
  Serial.print(F("STOP: ")); Serial.print(why);
  Serial.print(F(", count ")); Serial.print(counter);
  Serial.print(F(", ")); Serial.print((millis() - runStartMs) / 1000.0, 1); Serial.println(F(" s"));
}

static void startRun() {
  delay(START_DELAY_MS);
  odomReset(); pidReset(); counter = 0;
  servoGo(SERVO_HOME_DEG);
  runStartMs = millis(); tCtl = micros();
  go(ST_FOLLOW);
}

// ---------- 3. Головна логіка: один крок кожні 4 мс ----------
static uint8_t markCnt = 0;

static void control() {
  sensorsRead();
  if (millis() - runStartMs > RUN_TIMEOUT_MS) { finish(F("timeout")); return; }

  switch (st) {
    case ST_FOLLOW:
      // TODO: умова події завдання дня
      if (confirmed(isBlack(S_MARK), markCnt, 3)) { motorsBrake(); go(ST_ACTION); break; }
      // TODO: умова фінішу
      if (isWhite(S_LINE) && inState() > 500) { finish(F("line lost")); break; }
      followLine(S.basePwm);
      break;

    case ST_ACTION:
      // TODO: дія механізму. Робот стоїть, тому delay тут допустимий.
      motorsCoast();
      servoGo(90); delay(300); servoGo(SERVO_HOME_DEG);
      counter++;
      pidReset();
      go(ST_FOLLOW);
      break;

    default: break;
  }

  if (logRun) {
    static uint8_t div = 0;
    if (++div >= 10) {      // кожні ~40 мс: t, мм, датчики, стан — зручно для Serial Plotter
      div = 0;
      Serial.print(millis() - runStartMs); Serial.print(',');
      Serial.print(odomMM(), 0);
      for (uint8_t i = 0; i < N_SENSORS; i++) { Serial.print(','); Serial.print(sVal[i]); }
      Serial.print(','); Serial.println(st);
    }
  }
}

// ---------- 4. Serial-меню (115200) — працює, коли робот стоїть ----------
static void help() {
  Serial.println(F("s датчики наживо | m тест моторів | e енкодери | v серво 90/0"));
  Serial.println(F("c калібрування | g старт | l лог | p налаштування | w зберегти | x дефолти"));
  Serial.println(F("P0.25 I0 D2 B140 — PID і швидкість;  U0=..U3= — свої числа (U0 500)"));
}

static void streamSensors() {
  while (Serial.available()) Serial.read();
  while (!Serial.available()) {
    sensorsRead();
    for (uint8_t i = 0; i < N_SENSORS; i++) {
      Serial.print(sRaw[i]); Serial.print('/'); Serial.print(sVal[i]); Serial.print(F("  "));
    }
    Serial.print(F("| mm ")); Serial.println(odomMM(), 0);
    delay(100);
  }
  while (Serial.available()) Serial.read();
}

static void serialMenu() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case 'h': case '?': help(); break;
    case 's': streamSensors(); break;
    case 'm':
      Serial.println(F("лівий")); motorsSet(150, 0); delay(1000); motorsCoast(); delay(300);
      Serial.println(F("правий")); motorsSet(0, 150); delay(1000); motorsCoast(); delay(300);
      Serial.println(F("обидва")); motorsSet(150, 150); delay(1000); motorsBrake(); delay(150); motorsCoast();
      break;
    case 'e': odomReset(); motorsSet(110, 110); delay(1500); motorsCoast();
      Serial.print(F("ticks L ")); Serial.print(odomTicks(0)); Serial.print(F(" R ")); Serial.println(odomTicks(1)); break;
    case 'v': servoGo(90); delay(400); servoGo(SERVO_HOME_DEG); break;
    case 'c': Serial.println(F("водіть датчиками над білим і чорним...")); if (sensorsCalibrate(CALIB_MS)) settingsSave(); break;
    case 'g': startRun(); break;
    case 'l': logRun = !logRun; Serial.println(logRun ? F("log on") : F("log off")); break;
    case 'p': settingsPrint(); break;
    case 'w': settingsSave(); Serial.println(F("збережено")); break;
    case 'x': settingsDefaults(); settingsSave(); settingsPrint(); break;
    case 'P': S.kp = Serial.parseFloat(); settingsPrint(); break;
    case 'I': S.ki = Serial.parseFloat(); settingsPrint(); break;
    case 'D': S.kd = Serial.parseFloat(); settingsPrint(); break;
    case 'B': S.basePwm = Serial.parseInt(); settingsPrint(); break;
    case 'U': { int i = Serial.parseInt(); int v = Serial.parseInt();
                if (i >= 0 && i < 4) S.user[i] = v;
                settingsPrint(); break; }
    default: break;
  }
}

// ======================================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(300);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  motorsBegin(); motorsCoast();
  sensorsBegin(); odomBegin();
  settingsLoad();
  servoBegin();
  Serial.println(F("\nsosny robot ready. h — команди"));
  settingsPrint();
}

void loop() {
  if (st == ST_IDLE || st == ST_DONE) {
    serialMenu();
    digitalWrite(PIN_LED, st == ST_DONE || (millis() % 1000) < 60);
    if (buttonPressed()) { if (st == ST_DONE) go(ST_IDLE); else startRun(); }
    return;
  }
  if (buttonPressed()) { finish(F("button")); return; }     // аварійна зупинка
  unsigned long now = micros();
  if (now - tCtl >= CONTROL_DT_US) { tCtl = now; control(); }
}
