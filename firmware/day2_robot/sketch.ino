// =====================================================================
//  sosny · BEST EC Day 2 (TD) · автономний робот-транспортер
//  ESP32 + L298N + 2×TT (танковий хід) + 4×TCRT5000 + HC-SR04 + SG90
//
//  SIM 1 → Wokwi: телеметрія в Serial, команди з Serial
//  SIM 0 → справжній робот: Wi-Fi точка "sosny-robot", сторінка 192.168.4.1
//
//  Команди в Serial (і в SIM, і на роботі):  s = старт, x = стоп, n3 = стоп на 3-й лінії
// =====================================================================
#define SIM 1

#include <Arduino.h>
#include <Preferences.h>
#if !SIM
#include <WiFi.h>
#include <WebServer.h>
#endif

// ---------------- піни (як на схемі day2_wiring.svg) ----------------
const int PIN_ENA = 25, PIN_IN1 = 26, PIN_IN2 = 27;   // M1 ліве
const int PIN_IN3 = 16, PIN_IN4 = 17, PIN_ENB = 18;   // M2 праве
const int PIN_L = 32, PIN_C = 33, PIN_R = 34;         // сенсори лінії (ADC1)
const int PIN_CROSS = 35;                             // 4-й сенсор: перпендикулярні лінії
const int PIN_BATT = 36;                              // VP, дільник 20k/10k
const int PIN_TRIG = 15, PIN_ECHO = 39;  /* D15, VN */               // HC-SR04 (ECHO через 1k/2k)
const int PIN_SERVO = 22, PIN_BUZZ = 4;
const int PIN_START = 13, PIN_STOP = 14, PIN_LED = 2;
const int PIN_LED_GO = 21, PIN_LED_STOP = 5;           // зовнішні: зелений «їду», червоний «стою» (через 220 Ом)

// ---------------- налаштування (підбирати на трасі) ----------------
int   WHITE = 400, BLACK = 3200;   // сирі значення TCRT на білому / на чорному
float ON_LINE = 0.5;               // «бачу чорне», 0..1
int   BASE = 120;                  // базова швидкість, 0..255
int   MAX_PWM = 150;               // ОБМЕЖЕННЯ швидкості (~60 %)
int   PIVOT = 110;                 // швидкість розвороту на місці
float KP = 70, KD = 40;            // PID по лінії
int   SLOW_CM = 35, STOP_CM = 15;  // зона сповільнення / аварійна зупинка
float SLOW_K = 0.4;                // множник швидкості в зоні сповільнення
unsigned long CROSS_LOCK_MS = 300; // після лінії ігнор, щоб не порахувати двічі
unsigned long CROSS_CONFIRM_MS = 200; // за цей час після смуги лінія має бути під C (інакше це кут)
unsigned long LOST_MS = 1500;      // скільки шукати лінію розворотом
int   SERVO_HOME = 0, SERVO_DUMP = 110;
// --- об'їзд перешкоди на лінії (часові параметри підібрати під розмір коробки) ---
bool  BYPASS_ON = true;
int   BYPASS_SIDE = 1;               // +1 об'їзд праворуч, -1 ліворуч
unsigned long WAIT_MS = 3000;        // стоїть довше → це не людина, об'їжджаємо
unsigned long T_TURN = 450;          // розворот від лінії (~60–90°)
int   ARC_IN = 60, ARC_OUT = 150;    // дуга навколо: внутрішнє / зовнішнє колесо
unsigned long T_ARC_MIN = 900;       // спершу відʼїхати від лінії, потім шукати її
unsigned long T_ARC_MAX = 6000;      // не знайшли лінію → LOST
unsigned long T_ALIGN_MAX = 1200;    // доворот на лінію

// ---------------- стан ----------------
enum State { IDLE, RUN, ESTOP, ARRIVED, UNLOAD, DONE, LOST, BYPASS };
const char *STATE_NAME[] = {"IDLE", "RUN", "ESTOP", "ARRIVED", "UNLOAD", "DONE", "LOST", "BYPASS"};
State state = IDLE;
unsigned long tState = 0;

int   target = 3;          // стоп на N-й перпендикулярній лінії
int   crosses = 0, obstacles = 0;
float dist = 999, prevPos = 0;
int   lastSide = 0;        // де востаннє бачили лінію: -1 ліво, +1 право
bool  pivoting = false, slowed = false;
unsigned long lastSeen = 0, lastCross = 0, clearSince = 0, segStart = 0, avgSeg = 0;
int   bpPhase = 0;          // 0 відвернути, 1 дуга, 2 доворот на лінію
unsigned long bpT = 0, blockedSince = 0, candEnd = 0;
bool  crossCand = false;
String lastEvent = "boot";
Preferences prefs;

#if !SIM
WebServer web(80);
#endif

// ---------------- PWM (працює на ядрі ESP32 2.x і 3.x) ----------------
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void pwmInit(int pin, int freq, int bits) { ledcAttach(pin, freq, bits); }
void pwmWrite(int pin, int duty) { ledcWrite(pin, duty); }
#else
int chOf[40]; int nextCh = 0;
void pwmInit(int pin, int freq, int bits) { ledcSetup(nextCh, freq, bits); ledcAttachPin(pin, nextCh); chOf[pin] = nextCh++; }
void pwmWrite(int pin, int duty) { ledcWrite(chOf[pin], duty); }
#endif

void servoWrite(int deg) {                      // 50 Гц, 16 біт
  long us = 500 + (long)deg * 2000 / 180;
  pwmWrite(PIN_SERVO, us * 65535L / 20000L);
}

// ---------------- мотори ----------------
void setMotor(int en, int a, int b, int v) {     // v: -255..255
  v = constrain(v, -255, 255);
  digitalWrite(a, v > 0);
  digitalWrite(b, v < 0);
  pwmWrite(en, abs(v));
}
void drive(int left, int right) {
  setMotor(PIN_ENA, PIN_IN1, PIN_IN2, left);
  setMotor(PIN_ENB, PIN_IN3, PIN_IN4, right);
}
void brake() {                                   // активне гальмо L298N: IN1=IN2=HIGH
  digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, HIGH);
  digitalWrite(PIN_IN3, HIGH); digitalWrite(PIN_IN4, HIGH);
  pwmWrite(PIN_ENA, 255); pwmWrite(PIN_ENB, 255);
}

// ---------------- сенсори ----------------
float sensor(int pin) {                          // 0 = біле, 1 = чорне
  float v = (analogRead(pin) - WHITE) / float(BLACK - WHITE);
  return constrain(v, 0.0f, 1.0f);
}
float readCm() {
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long t = pulseIn(PIN_ECHO, HIGH, 25000);
  return t ? t / 58.0 : 999;
}
float battV() { return analogReadMilliVolts(PIN_BATT) * 3 / 1000.0; }

// ---------------- зумер ----------------
void buzz(bool on) {
  static bool was = false;
  if (on == was) return;
  was = on;
#if SIM
  if (on) tone(PIN_BUZZ, 2000); else noTone(PIN_BUZZ);
#else
  digitalWrite(PIN_BUZZ, on);                    // активний зумер через 2N2222
#endif
}

// ---------------- логіка ----------------
void event(const String &e) { lastEvent = e; Serial.println("EVENT " + e); }
void setState(State s) { state = s; tState = millis(); }

void startRun() {
  crosses = 0; obstacles = 0; prevPos = 0; lastSide = 0; crossCand = false;
  lastSeen = segStart = millis(); avgSeg = 0;
  servoWrite(SERVO_HOME);
  setState(RUN); event("start, target=" + String(target));
}
void stopRun(const String &why) { brake(); setState(IDLE); event("stop: " + why); }
void setTarget(int n) {
  if (n < 1 || n > 50) return;
  target = n; prefs.putInt("n", n); event("target=" + String(n));
}

int progress() {
  if (state == DONE || state == ARRIVED || state == UNLOAD) return 100;
  if (target <= 0) return 0;
  float frac = 0;
  if (avgSeg > 0) frac = min((millis() - segStart) / float(avgSeg), 0.95f);
  return constrain(int((crosses + frac) * 100 / target), 0, 99);
}

// ---------------- об'їзд: відвернути → дуга навколо → знайти лінію → доворот ----------------
void startBypass() {
  setState(BYPASS); bpPhase = 0; bpT = millis(); event("bypass start");
}
void bypass() {
  unsigned long now = millis(), t = now - bpT;
  int s = BYPASS_SIDE;
  float l = sensor(PIN_L), c = sensor(PIN_C), r = sensor(PIN_R);
  bool anyLine = l > ON_LINE || c > ON_LINE || r > ON_LINE;
  if (bpPhase == 0) {                                   // розворот убік від лінії
    drive(s * PIVOT, -s * PIVOT);
    if (t > T_TURN) { bpPhase = 1; bpT = now; }
  } else if (bpPhase == 1) {                            // дуга назад до лінії
    if (dist < 8) { brake(); return; }                  // впритул щось на дузі — чекаємо (коробку збоку не чіпаємо)
    if (s > 0) drive(ARC_IN, ARC_OUT); else drive(ARC_OUT, ARC_IN);
    if (t > T_ARC_MIN && anyLine) { bpPhase = 2; bpT = now; event("bypass: line found"); }
    else if (t > T_ARC_MAX) { stopRun("bypass: no line"); setState(LOST); }
  } else {                                              // доворот, поки лінія під C
    drive(s * PIVOT, -s * PIVOT);
    if (c > ON_LINE || t > T_ALIGN_MAX) {
      prevPos = 0; lastSide = 0; lastSeen = now; lastCross = now; crossCand = false;   // перетин лінії не рахувати
      setState(RUN); event("bypass done");
    }
  }
}

void control() {
  unsigned long now = millis();

  if (state == ARRIVED) {                        // пауза → вивантаження
    brake();
    if (now - tState > 500) { setState(UNLOAD); servoWrite(SERVO_DUMP); event("unload"); }
    return;
  }
  if (state == UNLOAD) {
    brake();
    if (now - tState > 2000) servoWrite(SERVO_HOME);
    if (now - tState > 3000) { setState(DONE); event("done"); }
    return;
  }
  if (state == BYPASS) { bypass(); return; }
  if (state != RUN && state != ESTOP) { brake(); pivoting = slowed = false; return; }

  float l = sensor(PIN_L), c = sensor(PIN_C), r = sensor(PIN_R), x = sensor(PIN_CROSS);

  // --- перпендикулярна лінія: 4-й сенсор + L,C,R одночасно чорні ---
  // Кут лінії (поворот у бік 4-го сенсора) дає такий самий малюнок, тому
  // рахуємо тільки якщо ПІСЛЯ смуги лінія йде далі прямо під C.
  bool pat = x > ON_LINE && l > ON_LINE && c > ON_LINE && r > ON_LINE;
  if (pat && now - lastCross > CROSS_LOCK_MS) { crossCand = true; candEnd = 0; }
  else if (crossCand) {
    if (!candEnd) candEnd = now;                                  // смуга скінчилась
    if (c > ON_LINE && l < ON_LINE && r < ON_LINE) {              // лінія йде далі → перехрестя
      crossCand = false; lastCross = now; crosses++;
      unsigned long seg = now - segStart; segStart = now;
      avgSeg = avgSeg ? (avgSeg + seg) / 2 : seg;
      event("cross " + String(crosses) + "/" + String(target));
      if (crosses >= target) { brake(); setState(ARRIVED); event("arrived"); return; }
    } else if (now - candEnd > CROSS_CONFIRM_MS) { crossCand = false; event("corner, not counted"); }
  }

  // --- куди їхати ---
  int left = 0, right = 0;
  float sum = l + c + r;
  pivoting = false;
  slowed = dist < SLOW_CM;
  if (sum >= 0.3) {
    lastSeen = now;
    float pos = (r - l) / sum;                   // -1 лінія під L … +1 під R
    if (fabs(pos) > 0.2) lastSide = pos < 0 ? -1 : 1;
    float corr = KP * pos + KD * (pos - prevPos);
    prevPos = pos;
    int base = slowed ? BASE * SLOW_K : BASE;
    left  = constrain(int(base + corr), 0, MAX_PWM);   // лінія під L → ліве повільніше
    right = constrain(int(base - corr), 0, MAX_PWM);
  } else if (now - lastSeen < LOST_MS && lastSide != 0) {
    pivoting = true;                              // різкий поворот: крутимось туди, де бачили лінію
    left  = lastSide < 0 ? -PIVOT :  PIVOT;
    right = lastSide < 0 ?  PIVOT : -PIVOT;
  } else {
    stopRun("line lost"); setState(LOST); return;
  }

  // --- безпека: аварійна зупинка (розворот на місці дозволено) ---
  if (state == RUN && !pivoting && dist < STOP_CM) {
    obstacles++; clearSince = 0; blockedSince = now; setState(ESTOP); event("obstacle " + String(dist, 0) + " cm");
  }
  if (state == ESTOP) {
    if (BYPASS_ON && dist <= STOP_CM + 5 && now - blockedSince > WAIT_MS) { startBypass(); return; }
    if (dist > STOP_CM + 5) {
      if (!clearSince) clearSince = now;
      if (now - clearSince > 1000) { setState(RUN); event("path clear, resume"); }
    } else clearSince = 0;
    if (state == ESTOP && !pivoting) { brake(); return; }
  }
  drive(left, right);
}

void updateBuzzer() {
  unsigned long now = millis();
  if (state == ESTOP) buzz(true);
  else if ((state == RUN && slowed) || state == BYPASS) buzz(now % 500 < 100);
  else if (state == ARRIVED) buzz(now % 200 < 100);
  else buzz(false);
  digitalWrite(PIN_LED, (state == RUN || state == BYPASS) ? (now % 1000 < 500) : state != IDLE);
  bool moving = (state == RUN || state == BYPASS);
  digitalWrite(PIN_LED_GO, moving);
  digitalWrite(PIN_LED_STOP, !moving);
}

String telemetry() {
  char b[220];
  snprintf(b, sizeof b,
    "{\"state\":\"%s\",\"cross\":%d,\"target\":%d,\"progress\":%d,\"dist\":%.0f,"
    "\"obstacles\":%d,\"batt\":%.2f,\"event\":\"%s\"}",
    STATE_NAME[state], crosses, target, progress(), dist, obstacles, battV(), lastEvent.c_str());
  return String(b);
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n'); cmd.trim();
  if (cmd == "s") startRun();
  else if (cmd == "x") stopRun("serial");
  else if (cmd.startsWith("n")) setTarget(cmd.substring(1).toInt());
}

void handleButtons() {
  static unsigned long lock = 0;
  if (millis() - lock < 250) return;
  if (!digitalRead(PIN_STOP)) { stopRun("STOP button"); lock = millis(); }
  else if (!digitalRead(PIN_START) && (state == IDLE || state == DONE || state == LOST)) { startRun(); lock = millis(); }
}

#if !SIM
const char PAGE[] PROGMEM = R"html(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>sosny robot</title>
<style>body{font-family:sans-serif;margin:16px;max-width:420px}button{font-size:20px;padding:12px;margin:4px;width:46%}
#st{font-size:28px;font-weight:bold}.bar{height:20px;background:#ddd}.bar div{height:100%;background:#2e7d32}</style></head><body>
<h2>sosny · робот</h2><div id="st">-</div><div class="bar"><div id="pb" style="width:0"></div></div>
<p id="info"></p><p id="ev"></p>
<button onclick="go('/start')" style="background:#c8e6c9">Старт</button><button onclick="go('/stop')" style="background:#ffcdd2">СТОП</button>
<p>Стоп на лінії №: <input id="n" type="number" min="1" max="50" style="width:60px;font-size:18px"> <button onclick="go('/set?n='+n.value)" style="width:auto">OK</button></p>
<script>function go(u){fetch(u)}
async function t(){try{let d=await(await fetch('/status')).json();st.textContent=d.state+' · '+d.progress+'%';pb.style.width=d.progress+'%';
info.textContent='Лінії: '+d.cross+'/'+d.target+' · Перешкода: '+d.dist+' см · Зупинок: '+d.obstacles+' · АКБ: '+d.batt+' В';ev.textContent='Подія: '+d.event;
if(document.activeElement!==n)n.value=d.target}catch(e){st.textContent='немає зв\'язку'}}setInterval(t,500);t()</script></body></html>)html";

void webSetup() {
  WiFi.softAP("sosny-robot", "sosny2026");
  web.on("/", [] { web.send(200, "text/html", PAGE); });
  web.on("/status", [] { web.send(200, "application/json", telemetry()); });
  web.on("/start", [] { if (state == IDLE || state == DONE || state == LOST) startRun(); web.send(200, "text/plain", "ok"); });
  web.on("/stop", [] { stopRun("web"); web.send(200, "text/plain", "ok"); });
  web.on("/set", [] { setTarget(web.arg("n").toInt()); web.send(200, "text/plain", "ok"); });
  web.begin();
}
#endif

void setup() {
  Serial.begin(115200);
  int outs[] = {PIN_IN1, PIN_IN2, PIN_IN3, PIN_IN4, PIN_TRIG, PIN_BUZZ, PIN_LED, PIN_LED_GO, PIN_LED_STOP};
  for (int p : outs) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_START, INPUT_PULLUP); pinMode(PIN_STOP, INPUT_PULLUP);
  pwmInit(PIN_ENA, 1000, 8); pwmInit(PIN_ENB, 1000, 8); pwmInit(PIN_SERVO, 50, 16);
  analogReadResolution(12);
  servoWrite(SERVO_HOME); brake();

  prefs.begin("robot", false);
  target = prefs.getInt("n", 3);
#if !SIM
  webSetup();
#endif
  Serial.println("sosny robot ready. s=start x=stop nN=target");
  event("ready, target=" + String(target));
}

void loop() {
  static unsigned long tCtl = 0, tDist = 0, tTel = 0;
  unsigned long now = millis();
  handleSerial();
  handleButtons();
#if !SIM
  web.handleClient();
#endif
  if (now - tDist >= 60) { tDist = now; dist = readCm(); }
  if (now - tCtl >= 10) { tCtl = now; control(); }
  if (now - tTel >= 500) { tTel = now; Serial.println(telemetry()); }
  updateBuzzer();
}
