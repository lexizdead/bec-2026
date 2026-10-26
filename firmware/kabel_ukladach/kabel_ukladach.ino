// ======================================================================
//  Робот «Швидкісний кабель-укладач» — головна прошивка
//  Arduino Nano · L298N · 2×TT (передній привід) · 2×TCRT5000 · SG90 · енкодери
//
//  Логіка (автомат станів):
//    IDLE ──кнопка──► FOLLOW ──мітка──► APPROACH ──доїхав──► скидання ─┐
//                       ▲                                              │
//                       └──────────── ще є шканти ◄────────────────────┘
//                                     9 скинуто ──► TO_END ──лінія скінчилась──► DONE
//
//  FOLLOW   — PID по краю смужки (50 % чорного), біля очікуваної мітки повільніше.
//  мітка    — датчик міток чорний N циклів поспіль + датчик лінії теж чорний
//             (мітка перетинає лінію) + не ближче MARK_MIN_GAP_MM до попередньої.
//  APPROACH — їдемо ще DROP_OFFSET_MM за енкодером (щілина позаду датчика),
//             гальмуємо, серво повертає барабан, чекаємо падіння, їдемо далі.
//  Пропустив мітку (з енкодером) → скидає там, де вона мала бути (+500 мм).
//
//  Кнопка «Старт» на D4. Затиснута при ввімкненні → калібрування датчиків.
//  Натиснута під час їзди → аварійна зупинка.
//  Serial 115200: h — список команд (тести моторів, датчиків, енкодерів, серво, PID).
// ======================================================================
#include "config.h"
#include "motors.h"
#include "sensors.h"
#include "odometry.h"
#include "magazine.h"
#include "settings.h"

enum State : uint8_t { ST_IDLE, ST_FOLLOW, ST_APPROACH, ST_TO_END, ST_DONE };
static const char *const STATE_NAMES[] = {"IDLE", "FOLLOW", "APPROACH", "TO_END", "DONE"};

static State st = ST_IDLE;
static Reading rd;
static unsigned long tCtl = 0, runStartMs = 0;
static bool logRun = false;
static uint16_t logDiv = 0;

// ---- PID ----
static float integ = 0, prevErr = 0, heldCorr = 0;
static bool pidFresh = true;

// ---- мітки ----
static uint8_t markSamples = 0;
static bool haveMark = false;
static float lastMarkMM = 0;          // де була остання мітка (одометрія)
static unsigned long lastMarkMs = 0;
static float dropTargetMM = 0;
static unsigned long approachStartMs = 0;

// ---- втрата лінії ----
static bool lost = false;
static float lostMM = 0;
static unsigned long lostMs = 0;

// ======================================================================
static bool buttonPressed() {   // true один раз на натискання, з антибрязкотом
  static bool raw = HIGH, stable = HIGH;
  static unsigned long tChange = 0;
  bool v = digitalRead(PIN_BUTTON);
  if (v != raw) { raw = v; tChange = millis(); }
  if (millis() - tChange > 30 && v != stable) {
    stable = v;
    if (stable == LOW) return true;
  }
  return false;
}

static void pidReset() { integ = 0; heldCorr = 0; pidFresh = true; }

// Рух уздовж лінії з базовою швидкістю base
static void followStep(int base) {
  float err = rd.line - LINE_SETPOINT;   // > 0: під датчиком більше чорного
  if (pidFresh) { prevErr = err; pidFresh = false; }
  float corr;
  if (rd.mark > MARK_THR) {
    // над міткою датчик лінії бачить суцільне чорне — не смикаємо кермо
    corr = heldCorr;
  } else {
    integ = constrain(integ + err, -I_LIMIT, I_LIMIT);
    corr = S.kp * err + S.ki * integ + S.kd * (err - prevErr);
    prevErr = err;
    // згладжений курс: поодинокий ривок (лінія наїхала на мітку на цикл раніше) майже не впливає
    heldCorr += 0.1f * (corr - heldCorr);
  }
#if !LINE_ON_LEFT_EDGE
  corr = -corr;
#endif
  // датчик над лівим краєм: більше чорного = робот з'їхав управо → повертаємо вліво
  int l = constrain(base - (int)corr, 0, PWM_MAX);
  int r = constrain(base + (int)corr, 0, PWM_MAX);
  motorsSet(l, r);
}

static int cruisePwm() {
  if (!odomAvailable()) return S.basePwm;
  float next = haveMark ? lastMarkMM + MARK_SPACING_MM : FIRST_MARK_MM;
  return (odomMM() > next - SLOW_ZONE_MM) ? S.slowPwm : S.basePwm;
}

static bool markDetected() {
  bool hit = rd.mark > MARK_THR;
#if MARK_REQUIRE_LINE
  hit = hit && rd.line > LINE_CONFIRM_THR;
#endif
  if (!hit) { markSamples = 0; return false; }
  if (markSamples < 255) markSamples++;
  if (markSamples != MARK_MIN_SAMPLES) return false;   // один раз на мітку
  if (haveMark) {
    if (odomAvailable()) { if (odomMM() - lastMarkMM < MARK_MIN_GAP_MM) return false; }
    else if (millis() - lastMarkMs < MARK_MIN_GAP_MS) return false;
  }
  return true;
}

static void startApproach(float markPos, bool virtualMark) {
  haveMark = true;
  lastMarkMM = markPos;
  lastMarkMs = millis();
  dropTargetMM = markPos + S.dropOffsetMm - BRAKE_COMP_MM;
  approachStartMs = millis();
  st = ST_APPROACH;
  Serial.print(virtualMark ? F("MISS → virtual mark @") : F("MARK @"));
  Serial.print(markPos, 0); Serial.print(F(" mm, drop #")); Serial.println(magDropped() + 1);
}

// true, якщо лінії немає довше за ліміт
static bool lostFor(float mmLimit, unsigned long msLimit) {
  if (rd.line >= LOST_THR) { lost = false; return false; }
  if (!lost) { lost = true; lostMM = odomMM(); lostMs = millis(); return false; }
  if (odomAvailable()) return odomMM() - lostMM > mmLimit;
  return millis() - lostMs > msLimit;
}

static void finish(const __FlashStringHelper *why) {
  motorsBrake(); delay(150); motorsCoast();
  st = ST_DONE;
  Serial.print(F("STOP: ")); Serial.print(why);
  Serial.print(F(", drops ")); Serial.print(magDropped());
  Serial.print(F(", ")); Serial.print(odomMM(), 0); Serial.print(F(" mm, "));
  Serial.print((millis() - runStartMs) / 1000.0, 1); Serial.println(F(" s"));
}

static void doDrop() {
  motorsBrake(); delay(BRAKE_MS); motorsCoast();
  delay(80);                         // робот повністю зупинився
  magDropNext();
  Serial.print(F("DROP ")); Serial.print(magDropped());
  Serial.print(F(" @ ")); Serial.print(odomMM(), 0); Serial.println(F(" mm"));
  pidReset();
  markSamples = 0;
  st = magEmpty() ? ST_TO_END : ST_FOLLOW;
  tCtl = micros();
}

static void startRun() {
  if (magDropped() > 0) magHome();
  for (int i = 0; i < 5; i++) { digitalWrite(PIN_LED, HIGH); delay(START_DELAY_MS / 10); digitalWrite(PIN_LED, LOW); delay(START_DELAY_MS / 10); }
  odomReset(); pidReset();
  haveMark = false; markSamples = 0; lost = false;
  runStartMs = millis();
  st = ST_FOLLOW;
  tCtl = micros();
  Serial.println(F("GO"));
}

// Один цикл керування (кожні CONTROL_DT_US)
static void control() {
  sensorsRead(rd);
  if (millis() - runStartMs > RUN_TIMEOUT_MS) { finish(F("timeout")); return; }

  switch (st) {
    case ST_FOLLOW:
      if (markDetected()) { startApproach(odomMM(), false); followStep(S.approachPwm); break; }
#if FORCE_DROP_ON_MISS
      if (odomAvailable() && haveMark && odomMM() > lastMarkMM + MARK_SPACING_MM + MISS_WINDOW_MM) {
        startApproach(lastMarkMM + MARK_SPACING_MM, true);
        followStep(S.approachPwm);
        break;
      }
#endif
      if (lostFor(LOST_STOP_MM, LOST_STOP_MS)) { finish(F("line lost")); break; }
      followStep(cruisePwm());
      break;

    case ST_APPROACH: {
      bool arrived = odomAvailable()
                       ? (odomMM() >= dropTargetMM)
                       : (millis() - approachStartMs >= (unsigned long)S.dropDelayMs);
      if (arrived) { doDrop(); break; }
      if (lostFor(LOST_STOP_MM, LOST_STOP_MS)) { finish(F("line lost")); break; }
      followStep(S.approachPwm);
      break;
    }

    case ST_TO_END:
      if (lostFor(FINISH_LOST_MM, FINISH_LOST_MS)) { finish(F("finish")); break; }
      followStep(S.basePwm);
      break;

    default: break;
  }

  if (logRun && ++logDiv >= 10) {    // кожні ~40 мс, якщо USB під'єднано
    logDiv = 0;
    Serial.print(millis() - runStartMs); Serial.print(',');
    Serial.print(odomMM(), 0); Serial.print(',');
    Serial.print(rd.line); Serial.print(',');
    Serial.print(rd.mark); Serial.print(',');
    Serial.println(STATE_NAMES[st]);
  }
}

// ======================= Serial-меню (у стані IDLE/DONE) =====================
static void help() {
  Serial.println(F("--- команди ---"));
  Serial.println(F("s  датчики й енкодери наживо (будь-яка клавіша — стоп)"));
  Serial.println(F("m  тест моторів: лівий, правий, обидва вперед по 1 с"));
  Serial.println(F("e  тест енкодерів: колеса мають зробити рівно 1 оберт"));
  Serial.println(F("v  магазин: наступна позиція   0  магазин: на старт"));
  Serial.println(F("c  калібрування датчиків (6 с)  g  старт (як кнопка)"));
  Serial.println(F("p  налаштування   w  зберегти в EEPROM   x  скинути на дефолти"));
  Serial.println(F("l  лог під час їзди вкл/викл (t,mm,line,mark,state)"));
  Serial.println(F("P0.25 I0 D2 — PID;  B140 Z110 A100 — PWM крейсер/зона/доїзд"));
  Serial.println(F("O180 — мм від датчика міток до точки падіння;  T1200 — те саме в мс без енкодерів"));
}

static void streamSensors() {
  while (Serial.available()) Serial.read();
  Serial.println(F("lineRaw lineN | markRaw markN | encL encR mm"));
  while (!Serial.available()) {
    sensorsRead(rd);
    Serial.print(rd.lineRaw); Serial.print(' '); Serial.print(rd.line); Serial.print(F(" | "));
    Serial.print(rd.markRaw); Serial.print(' '); Serial.print(rd.mark); Serial.print(F(" | "));
    Serial.print(odomTicksL()); Serial.print(' '); Serial.print(odomTicksR()); Serial.print(' ');
    Serial.println(odomMM(), 0);
    delay(100);
  }
  while (Serial.available()) Serial.read();
}

static void motorTest() {
  Serial.println(F("лівий вперед")); motorsSet(150, 0); delay(1000); motorsCoast(); delay(400);
  Serial.println(F("правий вперед")); motorsSet(0, 150); delay(1000); motorsCoast(); delay(400);
  Serial.println(F("обидва вперед")); motorsSet(150, 150); delay(1000); motorsBrake(); delay(150); motorsCoast();
  Serial.println(F("Якщо колесо крутилось назад — MOTOR_x_INVERT 1 або поміняти дроти."));
}

static void encTest() {
  if (!odomAvailable()) { Serial.println(F("ENC_COUNT = 0")); return; }
  odomReset();
  motorsSet(110, 110);
  unsigned long t0 = millis();
  while (millis() - t0 < 4000) {
    bool doneL = odomTicksL() >= 2 * ENC_SLOTS;
    bool doneR = (ENC_COUNT < 2) || odomTicksR() >= 2 * ENC_SLOTS;
    motorsSet(doneL ? 0 : 110, doneR ? 0 : 110);
    if (doneL && doneR) break;
  }
  motorsCoast();
  Serial.print(F("ticks L ")); Serial.print(odomTicksL());
  Serial.print(F("  R ")); Serial.println(odomTicksR());
  Serial.println(F("Кожне колесо має зробити 1 оберт. Ні — перевірте ENC_SLOTS / диск / дроти."));
}

static void serialMenu() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case 'h': case '?': help(); break;
    case 's': streamSensors(); break;
    case 'm': motorTest(); break;
    case 'e': encTest(); break;
    case 'v': magDropNext(); Serial.print(F("mag pos ")); Serial.println(magDropped()); break;
    case '0': magHome(); Serial.println(F("mag home")); break;
    case 'c':
      Serial.println(F("Калібрування: датчики над БІЛИМ, далі водіть через лінію й мітку..."));
      delay(500);
      if (sensorsCalibrate(CALIB_MS)) { settingsSave(); Serial.println(F("збережено")); }
      break;
    case 'g': startRun(); break;
    case 'p': settingsPrint(); break;
    case 'w': settingsSave(); Serial.println(F("збережено в EEPROM")); break;
    case 'x': settingsDefaults(); settingsSave(); Serial.println(F("дефолти")); break;
    case 'l': logRun = !logRun; Serial.println(logRun ? F("log on") : F("log off")); break;
    case 'P': S.kp = Serial.parseFloat(); settingsPrint(); break;
    case 'I': S.ki = Serial.parseFloat(); settingsPrint(); break;
    case 'D': S.kd = Serial.parseFloat(); settingsPrint(); break;
    case 'B': S.basePwm = Serial.parseInt(); settingsPrint(); break;
    case 'Z': S.slowPwm = Serial.parseInt(); settingsPrint(); break;
    case 'A': S.approachPwm = Serial.parseInt(); settingsPrint(); break;
    case 'O': S.dropOffsetMm = Serial.parseInt(); settingsPrint(); break;
    case 'T': S.dropDelayMs = Serial.parseInt(); settingsPrint(); break;
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
  sensorsBegin();
  odomBegin();
  settingsLoad();
  magBegin();
  Serial.println(F("\nКабель-укладач готовий. h — команди"));
  settingsPrint();

  if (digitalRead(PIN_BUTTON) == LOW) {   // кнопка затиснута при ввімкненні
    Serial.println(F("Калібрування: відпустіть кнопку; датчики над БІЛИМ, далі водіть через лінію й мітку"));
    while (digitalRead(PIN_BUTTON) == LOW) delay(10);
    delay(400);
    if (sensorsCalibrate(CALIB_MS)) settingsSave();
    else for (int i = 0; i < 20; i++) { digitalWrite(PIN_LED, i & 1); delay(50); }   // помилка
  }
}

void loop() {
  if (st == ST_IDLE || st == ST_DONE) {
    serialMenu();
    if (st == ST_IDLE) digitalWrite(PIN_LED, (millis() % 1000) < 60);   // короткий спалах
    else digitalWrite(PIN_LED, HIGH);                                   // DONE: горить
    if (buttonPressed()) {
      if (st == ST_DONE) st = ST_IDLE;
      else startRun();
    }
    return;
  }
  if (buttonPressed()) { finish(F("button")); return; }   // аварійна зупинка
  unsigned long now = micros();
  if (now - tCtl >= CONTROL_DT_US) { tCtl = now; control(); }
}
