// ======================================================================
//  robot.ino — ЛОГІКА ДНЯ 2 (автомат станів) · sosny · BEST EC TD
//
//  Робот їде по чорній лінії, рахує поперечні лінії (4-й сенсор) і на N-й
//  зупиняється й вивантажує кузов. Перешкода: < 35 см — сповільнення,
//  < 15 см — стоп; стоїть > 3 с — об'їзд і назад на лінію.
//  Телеметрія: сторінка 192.168.4.1 (Wi-Fi «sosny-robot») + JSON у Serial.
//
//  Логіка = firmware/day2_robot/sketch.ino з чату Віталія (той файл — для
//  Wokwi). Тут те саме, розкладене по шаблону: config / hw / net / robot.
//  Новe порівняно з sketch.ino: калібрування кожного датчика окремо,
//  «пройдено, см», журнал подій, налаштування з Serial без перепрошивки.
// ======================================================================
#include "config.h"
#include "hw.h"
#include "net.h"
#include <WiFi.h>

// ---------- 1. Стани ----------
enum State : uint8_t { IDLE, RUN, ESTOP, BYPASS, ARRIVED, UNLOAD, DONE, LOST };
static const char *const STATE_NAME[] = {"IDLE", "RUN", "ESTOP", "BYPASS", "ARRIVED", "UNLOAD", "DONE", "LOST"};
static State st = IDLE;
static unsigned long tState = 0, runStart = 0;

static int   crosses = 0, obstacles = 0, bypasses = 0;
static float dist = 999, pathMm = 0, prevPos = 0;
static int   lastSide = 0;                 // де востаннє бачили лінію: -1 ліво, +1 право
static bool  pivoting = false, slowed = false, crossCand = false;
static unsigned long lastSeen = 0, lastCross = 0, candEnd = 0, clearSince = 0, blockedSince = 0;
static unsigned long segStart = 0, avgSeg = 0;
static int   bpPhase = 0; static unsigned long bpT = 0;
static int   curL = 0, curR = 0;           // що зараз подано на мотори (для «пройдено»)
static int   turnDir = 0;                  // поворотник: -1 ліво, 0 нема, +1 право
static unsigned long turnUntil = 0;
static void setTurn(int d) { if (d) { turnDir = d; turnUntil = millis() + TURN_HOLD_MS; } }

// ---------- журнал подій (останні 8 — у телеметрію) ----------
static String evLog[8]; static uint8_t evHead = 0;
static void event(const String &e) {
  String line = String((millis() - runStart) / 1000.0, 1) + "с " + e;
  evLog[evHead] = line; evHead = (evHead + 1) % 8;
  Serial.println("EVENT " + line);
}
static void go(State s) { st = s; tState = millis(); }
static unsigned long inState() { return millis() - tState; }
static float sv(uint8_t i) { return sVal[i] / 1000.0f; }     // 0..1
static const float LINE = ON_LINE / 1000.0f;

static void drive(int l, int r) { curL = l; curR = r; motorsSet(l, r); }
static void stopMotors() { curL = curR = 0; motorsBrake(); }

// ---------- 2. Команди (кнопки, Serial, веб) ----------
void cmdStart() {
  if (!(st == IDLE || st == DONE || st == LOST)) return;
  crosses = obstacles = bypasses = 0; pathMm = 0; prevPos = 0; lastSide = 0; crossCand = false;
  runStart = lastSeen = segStart = millis(); avgSeg = 0;
  servoWrite(SERVO_HOME);
  go(RUN); event("старт, ціль — лінія " + String(S.target));
}
void cmdStop(const char *why) { stopMotors(); go(IDLE); event(String("стоп: ") + why); }
void cmdTarget(int n) {
  if (n < 1 || n > 50) return;
  S.target = n; settingsSave(); event("ціль = " + String(n));
}

static int progress() {
  if (st == ARRIVED || st == UNLOAD || st == DONE) return 100;
  float frac = avgSeg ? min((millis() - segStart) / float(avgSeg), 0.95f) : 0;
  return constrain(int((crosses + frac) * 100 / S.target), 0, 99);
}

// ---------- 3. Об'їзд: відвернути → дуга → знайти лінію → доворот ----------
static void bypass() {
  unsigned long now = millis(), t = now - bpT;
  int s = S.bpSide;
  bool anyLine = isBlack(S_L) || isBlack(S_C) || isBlack(S_R);
  if (bpPhase == 0) {                                       // відвернути від лінії
    drive(s * S.pivot, -s * S.pivot); setTurn(s);
    if (t > (unsigned long)S.tTurn) { bpPhase = 1; bpT = now; }
  } else if (bpPhase == 1) {                                // дуга навколо коробки
    if (dist < 8) { stopMotors(); return; }                 // впритул щось — чекаємо
    if (s > 0) drive(S.arcIn, S.arcOut); else drive(S.arcOut, S.arcIn);
    setTurn(-s);                                            // дуга — назад у бік лінії
    if (t > (unsigned long)S.tArcMin && anyLine) { bpPhase = 2; bpT = now; event("об'їзд: лінію знайдено"); }
    else if (t > (unsigned long)S.tArcMax) { stopMotors(); go(LOST); event("об'їзд: лінію не знайдено"); }
  } else {                                                  // доворот, поки лінія під C
    drive(s * S.pivot, -s * S.pivot); setTurn(s);
    if (isBlack(S_C) || t > (unsigned long)S.tAlign) {
      prevPos = 0; lastSide = 0; lastSeen = lastCross = now; crossCand = false;   // перетин лінії не рахувати
      go(RUN); event("об'їзд завершено");
    }
  }
}

// ---------- 4. Головна логіка: крок кожні 10 мс ----------
static void control() {
  unsigned long now = millis();
  sensorsRead();

  // «пройдено»: енкодерів нема → час × швидкість (мм/с при BASE, заміряти один раз)
  int fwd = max(0, (curL + curR) / 2);
  pathMm += fwd / float(S.base) * S.mmPerS * CONTROL_MS / 1000.0f;

  if (st == RUN && now - runStart > RUN_TIMEOUT_MS) { cmdStop("таймаут"); return; }
  if (st == ARRIVED) { stopMotors(); if (inState() > 500) { go(UNLOAD); servoWrite(SERVO_DUMP); event("вивантаження"); } return; }
  if (st == UNLOAD)  { stopMotors(); if (inState() > 2000) servoWrite(SERVO_HOME);
                       if (inState() > 3000) { go(DONE); event("готово"); } return; }
  if (st == BYPASS)  { bypass(); return; }
  if (st != RUN && st != ESTOP) { stopMotors(); pivoting = slowed = false; return; }

  float l = sv(S_L), c = sv(S_C), r = sv(S_R), x = sv(S_X);

  // --- поперечна лінія: 4-й + L,C,R одночасно; рахуємо, якщо далі лінія під C (а не кут) ---
  bool pat = x > LINE && l > LINE && c > LINE && r > LINE;
  if (pat && now - lastCross > CROSS_LOCK_MS) { crossCand = true; candEnd = 0; }
  else if (crossCand) {
    if (!candEnd) candEnd = now;
    if (c > LINE && l < LINE && r < LINE) {
      crossCand = false; lastCross = now; crosses++;
      unsigned long seg = now - segStart; segStart = now;
      avgSeg = avgSeg ? (avgSeg + seg) / 2 : seg;
      event("лінія " + String(crosses) + "/" + String(S.target));
      if (crosses >= S.target) { stopMotors(); go(ARRIVED); event("прибув у т. Б"); return; }
    } else if (now - candEnd > CROSS_CONFIRM_MS) { crossCand = false; event("кут, не рахуємо"); }
  }

  // --- рух по лінії: PD по положенню лінії під L/C/R ---
  int left = 0, right = 0;
  float sum = l + c + r;
  pivoting = false;
  slowed = dist < SLOW_CM;
  if (sum >= 0.3f) {
    lastSeen = now;
    float pos = (r - l) / sum;                    // -1 лінія під L … +1 під R
    if (fabs(pos) > 0.2f) lastSide = pos < 0 ? -1 : 1;
    if (fabs(pos) > TURN_POS) setTurn(pos < 0 ? -1 : 1);   // лінія під L → повертаємо ліворуч
    float corr = S.kp * pos + S.kd * (pos - prevPos);
    prevPos = pos;
    int base = slowed ? S.base * SLOW_K : S.base;
    left  = constrain(int(base + corr), 0, PWM_LIMIT);
    right = constrain(int(base - corr), 0, PWM_LIMIT);
  } else if (now - lastSeen < LOST_MS && lastSide != 0) {
    pivoting = true;                               // різкий поворот: крутимось туди, де бачили лінію
    setTurn(lastSide);
    left  = lastSide < 0 ? -S.pivot :  S.pivot;
    right = lastSide < 0 ?  S.pivot : -S.pivot;
  } else { stopMotors(); go(LOST); event("лінію втрачено"); return; }

  // --- безпека: аварійна зупинка (розворот на місці дозволено — коробки біля поворотів) ---
  if (st == RUN && !pivoting && dist < STOP_CM) {
    obstacles++; clearSince = 0; blockedSince = now; go(ESTOP); event("перешкода " + String(dist, 0) + " см");
  }
  if (st == ESTOP) {
    if (S.bypass && dist <= STOP_CM + 5 && now - blockedSince > (unsigned long)S.waitMs) {
      bypasses++; go(BYPASS); bpPhase = 0; bpT = now; event("об'їзд"); return;
    }
    if (dist > STOP_CM + 5) {
      if (!clearSince) clearSince = now;
      if (now - clearSince > RESUME_MS) { go(RUN); event("шлях вільний"); }
    } else clearSince = 0;
    if (st == ESTOP && !pivoting) { stopMotors(); return; }
  }
  drive(left, right);
}

// ---------- 5. Телеметрія (і для сторінки, і для Serial) ----------
String telemetryJson() {
  String j = "{\"state\":\"" + String(STATE_NAME[st]) + "\"";
  j += ",\"cross\":" + String(crosses) + ",\"target\":" + String(S.target);
  j += ",\"progress\":" + String(progress());
  j += ",\"path_cm\":" + String(pathMm / 10, 0);
  j += ",\"time_s\":" + String(st == IDLE ? 0 : (millis() - runStart) / 1000);
  j += ",\"dist\":" + String(dist, 0);
  j += ",\"obstacles\":" + String(obstacles) + ",\"bypasses\":" + String(bypasses);
  j += ",\"batt\":" + String(battV(), 2);
  j += ",\"heap\":" + String(ESP.getFreeHeap() / 1024) + ",\"wifi\":" + String(WiFi.softAPgetStationNum()) + ",\"turn\":" + String(turnDir);   // діагностика: КБ вільної пам'яті, клієнтів Wi-Fi
  j += ",\"sens\":[" + String(sVal[0]) + "," + String(sVal[1]) + "," + String(sVal[2]) + "," + String(sVal[3]) + "]";
  j += ",\"log\":[";
  for (uint8_t k = 0; k < 8; k++) {
    const String &e = evLog[(evHead + 7 - k) % 8];      // новіші — першими
    if (!e.length()) break;
    if (k) j += ",";
    j += "\"" + e + "\"";
  }
  return j + "]}";
}

// ---------- 6. Зумер / світлодіод ----------
static void signals() {
  unsigned long now = millis();
  if (st == ESTOP) buzz(true);
  else if ((st == RUN && slowed) || st == BYPASS) buzz(now % 500 < 100);
  else if (st == ARRIVED) buzz(now % 200 < 100);
  else if (turnDir) { unsigned long ph = now % TURN_HALF_MS; buzz(ph < 15); }   // «тік-так» як у машині
  else buzz(false);
  bool moving = (st == RUN || st == BYPASS);
  digitalWrite(PIN_LED, moving ? (now % 1000 < 500) : st != IDLE);
  digitalWrite(PIN_LED_GO, moving);          // для людей навколо: зелений = їду
  digitalWrite(PIN_LED_STOP, !moving);       //                    червоний = стою
  // поворотники: блимають у бік повороту, гаснуть через TURN_HOLD_MS після нього
  if (!moving || now > turnUntil) turnDir = 0;
  bool blink = (now / TURN_HALF_MS) % 2 == 0;
  digitalWrite(PIN_TURN_L, turnDir < 0 && blink);
  digitalWrite(PIN_TURN_R, turnDir > 0 && blink);
}

// ---------- 7. Кнопки ----------
static void buttons() {
  static unsigned long lock = 0;
  if (millis() - lock < 250) return;
  if (!digitalRead(PIN_STOP)) { cmdStop("кнопка STOP"); lock = millis(); }
  else if (!digitalRead(PIN_START)) { cmdStart(); lock = millis(); }
}

// ---------- 8. Serial-меню (115200) ----------
static void help() {
  Serial.println(F("g старт | x стоп | n3 ціль | s датчики | m мотори | u відстань | v серво | c калібрування"));
  Serial.println(F("p налаштування | w зберегти | r дефолти | t телеметрія вкл/викл"));
  Serial.println(F("P70 D40 B120 V110 — PD, база, розворот;  M250 — мм/с;  O1/O0 об'їзд;  Q1/Q-1 бік"));
  Serial.println(F("W3000 T450 I60 E150 A900 Z6000 L1200 — об'їзд: чекати, відвернути, дуга вн/зовн, мін/макс, доворот"));
}
static bool telemOn = true;
static void serialMenu() {
  if (!Serial.available()) return;
  char c = Serial.read();
  bool idle = (st == IDLE || st == DONE || st == LOST);
  switch (c) {
    case 'h': case '?': help(); break;
    case 'g': cmdStart(); break;
    case 'x': cmdStop("serial"); break;
    case 'n': cmdTarget(Serial.parseInt()); break;
    case 't': telemOn = !telemOn; break;
    case 's': if (idle) { for (int k = 0; k < 30; k++) { sensorsRead();
                Serial.printf("L %4d/%4d  C %4d/%4d  R %4d/%4d  X %4d/%4d\n", sRaw[0], sVal[0], sRaw[1], sVal[1], sRaw[2], sVal[2], sRaw[3], sVal[3]);
                delay(100); } } break;
    case 'm': if (idle) {
                Serial.println(F("ліве вперед")); motorsSet(140, 0); delay(800); motorsBrake(); delay(300);
                Serial.println(F("праве вперед")); motorsSet(0, 140); delay(800); motorsBrake(); delay(300);
                Serial.println(F("розворот")); motorsSet(120, -120); delay(600); motorsBrake(); } break;
    case 'u': for (int k = 0; k < 10; k++) { Serial.printf("%.0f см\n", distCm()); delay(100); } break;
    case 'v': if (idle) { servoWrite(SERVO_DUMP); delay(1500); servoWrite(SERVO_HOME); } break;
    case 'c': if (idle) { Serial.println(F("5 с водіть датчиками над білим і чорним...")); if (sensorsCalibrate(CALIB_MS)) { settingsSave(); Serial.println(F("збережено")); } } break;
    case 'p': settingsPrint(); break;
    case 'w': settingsSave(); Serial.println(F("збережено")); break;
    case 'r': settingsDefaults(); settingsSave(); settingsPrint(); break;
    case 'P': S.kp = Serial.parseFloat(); settingsPrint(); break;
    case 'D': S.kd = Serial.parseFloat(); settingsPrint(); break;
    case 'B': S.base = constrain(Serial.parseInt(), 40, PWM_LIMIT); settingsPrint(); break;
    case 'V': S.pivot = constrain(Serial.parseInt(), 40, PWM_LIMIT); settingsPrint(); break;
    case 'M': S.mmPerS = Serial.parseInt(); settingsPrint(); break;
    case 'O': S.bypass = Serial.parseInt() != 0; settingsPrint(); break;
    case 'Q': S.bpSide = Serial.parseInt() < 0 ? -1 : 1; settingsPrint(); break;
    case 'W': S.waitMs = Serial.parseInt(); settingsPrint(); break;
    case 'T': S.tTurn = Serial.parseInt(); settingsPrint(); break;
    case 'I': S.arcIn = Serial.parseInt(); settingsPrint(); break;
    case 'E': S.arcOut = Serial.parseInt(); settingsPrint(); break;
    case 'A': S.tArcMin = Serial.parseInt(); settingsPrint(); break;
    case 'Z': S.tArcMax = Serial.parseInt(); settingsPrint(); break;
    case 'L': S.tAlign = Serial.parseInt(); settingsPrint(); break;
    default: break;
  }
}

// ======================================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(300);
  pinMode(PIN_TRIG, OUTPUT); pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZ, OUTPUT); pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_LED_GO, OUTPUT); pinMode(PIN_LED_STOP, OUTPUT);
  pinMode(PIN_TURN_L, OUTPUT); pinMode(PIN_TURN_R, OUTPUT);
  pinMode(PIN_START, INPUT_PULLUP); pinMode(PIN_STOP, INPUT_PULLUP);
  settingsLoad();
  motorsBegin(); motorsBrake();
  sensorsBegin();
  servoBegin();
  netBegin();
  Serial.println(F("\nsosny robot (day 2) ready. h — команди"));
  settingsPrint();
  event("готовий, ціль — лінія " + String(S.target));
}

void loop() {
  static unsigned long tCtl = 0, tDist = 0, tTel = 0;
  unsigned long now = millis();
  serialMenu();
  buttons();
  netLoop();
  if (now - tDist >= DIST_MS)    { tDist = now; dist = distCm(); }
  if (now - tCtl  >= CONTROL_MS) { tCtl = now; control(); }
  if (telemOn && now - tTel >= TELEM_MS) { tTel = now; Serial.println(telemetryJson()); }
  signals();
}
