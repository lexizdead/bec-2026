// sosny — робот з ламаною рамою (як Т-150К), об'їзд перешкод
// Arduino Nano + L298N + SG90 (шарнір рами) + HC-SR04 (на передній половині)
//
// Логіка:
//   1. Чекаємо кнопку Start.
//   2. Їдемо прямо. Перешкода ближче SLOW_CM -> повільніше.
//   3. Ближче STOP_CM -> стоп, трохи назад, ламаємо раму вбік і їдемо,
//      поки попереду не стане вільно; потім рівно; потім ламаємо в інший бік
//      стільки ж часу, щоб повернутись на початковий курс.
//   4. Якщо вбік теж глухо -> пробуємо інший бік.
//
// Живлення серво — від DC-DC 5 V, НЕ від 5V Nano. GND усіх модулів спільний.

#include <Servo.h>

// ---------- Піни (як у 1-й день) ----------
const uint8_t PIN_ENA = 5, PIN_IN1 = 7, PIN_IN2 = 8;    // лівий мотор
const uint8_t PIN_ENB = 6, PIN_IN3 = 11, PIN_IN4 = 12;  // правий мотор
const uint8_t PIN_SERVO = 9;
const uint8_t PIN_START = 2;                            // кнопка на GND
const uint8_t PIN_TRIG = A1, PIN_ECHO = A2;             // HC-SR04

// ---------- Налаштування ----------
const int SERVO_CENTER = 90;   // підібрати так, щоб рама стояла рівно
const int STEER_MAX    = 30;   // ±30°, як у Т-150К; більше не ставити
const bool SERVO_INVERT = false; // якщо "вліво" ламає вправо -> true

const int SPEED_FWD  = 170;    // 0..255
const int SPEED_SLOW = 120;
const int SPEED_TURN = 150;    // на повороті (раму ламає легше в русі)

const int SLOW_CM  = 40;
const int STOP_CM  = 20;
const int CLEAR_CM = 45;       // "вільно" для виходу з об'їзду

const unsigned long BACK_MS      = 350;   // відкат назад
const unsigned long EXTRA_MS     = 400;   // ще трохи вбік після "вільно"
const unsigned long STRAIGHT_MS  = 700;   // рівно повз перешкоду
const unsigned long SIDE_MAX_MS  = 2500;  // якщо за цей час не вільно -> інший бік
const unsigned long STEER_SETTLE = 250;   // час, щоб серво доїхало

Servo steer;

// ---------- Рух ----------
void motor(uint8_t en, uint8_t a, uint8_t b, int spd) {
  spd = constrain(spd, -255, 255);
  digitalWrite(a, spd > 0);
  digitalWrite(b, spd < 0);
  analogWrite(en, abs(spd));
}
void drive(int spd) {                 // обидва мотори однаково — кермує рама
  motor(PIN_ENA, PIN_IN1, PIN_IN2, spd);
  motor(PIN_ENB, PIN_IN3, PIN_IN4, spd);
}
void stopAll() { drive(0); }

// angle: -STEER_MAX (вліво) .. 0 .. +STEER_MAX (вправо)
void setSteer(int angle) {
  angle = constrain(angle, -STEER_MAX, STEER_MAX);
  if (SERVO_INVERT) angle = -angle;
  steer.write(SERVO_CENTER + angle);
}

// ---------- Далекомір ----------
int readCmOnce() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 25000UL);   // ~4 м
  if (us == 0) return 400;                               // нічого не бачить
  return us / 58;
}
int readCm() {                        // медіана з 3 — менше хибних спрацювань
  int a = readCmOnce(); delay(8);
  int b = readCmOnce(); delay(8);
  int c = readCmOnce();
  if (a > b) { int t = a; a = b; b = t; }
  if (b > c) { int t = b; b = c; c = t; }
  if (a > b) { int t = a; a = b; b = t; }
  return b;
}

// ---------- Об'їзд ----------
int lastSide = 1;                     // чергуємо боки: +1 вправо, -1 вліво

// Їдемо зламаною рамою в бік side, поки не стане вільно.
// Повертає час, витрачений на бокову фазу, або 0 якщо не вийшло.
unsigned long goSide(int side) {
  setSteer(side * STEER_MAX);
  delay(STEER_SETTLE);
  drive(SPEED_TURN);
  unsigned long t0 = millis();
  while (millis() - t0 < SIDE_MAX_MS) {
    int d = readCm();
    if (d < STOP_CM) break;           // і тут глухо
    if (d > CLEAR_CM) {
      delay(EXTRA_MS);
      return millis() - t0;
    }
  }
  stopAll();
  return 0;
}

void avoid() {
  stopAll();
  setSteer(0);
  delay(STEER_SETTLE);
  drive(-SPEED_SLOW);                 // відкат назад рівно
  delay(BACK_MS);
  stopAll();

  int side = -lastSide;
  unsigned long tSide = goSide(side);
  if (tSide == 0) {                   // не вийшло — назад і в інший бік
    setSteer(0); delay(STEER_SETTLE);
    drive(-SPEED_SLOW); delay(BACK_MS * 2); stopAll();
    side = -side;
    tSide = goSide(side);
    if (tSide == 0) { stopAll(); setSteer(0); return; }  // залишаємось стояти
  }
  lastSide = side;

  setSteer(0); delay(STEER_SETTLE);   // повз перешкоду рівно
  drive(SPEED_FWD);
  delay(STRAIGHT_MS);

  setSteer(-side * STEER_MAX);        // повертаємось на курс
  delay(STEER_SETTLE);
  drive(SPEED_TURN);
  delay(tSide);
  setSteer(0);
}

// ---------- Основне ----------
void setup() {
  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT); pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENA, OUTPUT); pinMode(PIN_ENB, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT); pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_START, INPUT_PULLUP);
  stopAll();

  steer.attach(PIN_SERVO);
  setSteer(0);

  Serial.begin(9600);
  Serial.println(F("sosny: press Start"));
  while (digitalRead(PIN_START) == HIGH) delay(10);
  delay(500);                         // прибрати руку
}

void loop() {
  int d = readCm();
  Serial.println(d);

  if (d < STOP_CM) {
    avoid();
  } else if (d < SLOW_CM) {
    setSteer(0);
    drive(SPEED_SLOW);
  } else {
    setSteer(0);
    drive(SPEED_FWD);
  }
  delay(20);
}
