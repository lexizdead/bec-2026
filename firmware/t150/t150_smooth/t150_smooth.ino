// sosny — Т-150: плавний рух, плавна серва, об'їзд перешкоди справа.
// Мотор L298N OUT3/OUT4 (ENB D6, IN3 D11, IN4 D12). Серво рами D9.
// HC-SR04: лівий A0/A1, передній A2/A3, правий A4/A5. Без кнопки: старт через 3 с.
//
// Спереду: від SLOW_CM до STOP_CM швидкість плавно падає, на STOP_CM (5 см) — стоп.
//          Перешкоду прибрали -> плавно розганяється.
// Справа ближче AVOID_CM -> рама плавно ламається вліво, об'їжджає; праворуч вільно ->
//          ще HOLD_MS вліво (щоб відійти), потім плавно рівно. Зліва — так само, дзеркально.
// Датчик не працює (5 поганих вимірів поспіль) -> ігнорується, робот просто їде вперед.
// Світлодіод L: блимає = старт через 3 с, горить = їде, не горить = стоїть.
#include <Servo.h>

// ---- піни ----
const int ENB = 6, IN3 = 11, IN4 = 12;
const int SERVO_PIN = 9;
const int TRIG[3] = {A0, A2, A4};   // лівий, передній, правий
const int ECHO[3] = {A1, A3, A5};
enum { LEFT = 0, FRONT = 1, RIGHT = 2 };

// ---- рух ----
const int  SPEED = 170;            // крейсерська швидкість 0..255
const int  MIN_SPEED = 90;         // нижче TT-мотор зазвичай не крутиться
const int  ACCEL = 4;              // на скільки міняється швидкість кожні 20 мс (менше = плавніше)
const int  STOP_CM = 5;            // тут стоїть
const int  SLOW_CM = 35;           // звідси починає гальмувати
const bool MOTOR_INVERT = false;   // їде назад замість вперед -> true

// ---- об'їзд ----
const int  AVOID_CM = 25;          // перешкода збоку ближче -> об'їжджаємо
const int  FRAME_AVOID = 25;       // на скільки ламаємо раму при об'їзді, °
const unsigned long HOLD_MS = 600; // ще стільки тримаємо поворот після того, як збоку стало вільно

// ---- серво ----
const int  SERVO_CENTER = 90;      // рама рівно (підібрати 85..95)
const int  FRAME_MAX = 30;
const bool SERVO_INVERT = false;   // ламає не в той бік -> true
const unsigned long SERVO_STEP_MS = 15;   // 1° за стільки мс (більше = плавніше)

// ---- стан ----
Servo servo;
int dist[3] = {400, 400, 400};
int bad[3] = {0, 0, 0}, good[3] = {0, 0, 0};
bool alive[3] = {true, true, true};
int sensorIdx = 0;
unsigned long tSensor = 0, tServo = 0, tSpeed = 0, tPrint = 0, holdUntil = 0;
int servoNow = SERVO_CENTER, servoTarget = SERVO_CENTER, frame = 0, holdFrame = 0;
int speedNow = 0, speedTarget = 0;

void motor(int spd) {
  if (MOTOR_INVERT) spd = -spd;
  digitalWrite(IN3, spd > 0);
  digitalWrite(IN4, spd < 0);
  analogWrite(ENB, abs(spd));
}

// TRIG виходом тільки на час імпульсу — плата не падає при кривій проводці датчика
int measure(int i) {
  pinMode(TRIG[i], OUTPUT);
  digitalWrite(TRIG[i], LOW);  delayMicroseconds(2);
  digitalWrite(TRIG[i], HIGH); delayMicroseconds(10);
  digitalWrite(TRIG[i], LOW);
  pinMode(TRIG[i], INPUT);
  long us = pulseIn(ECHO[i], HIGH, 20000);
  return us ? us / 58 : -1;
}

void updateSensors(unsigned long now) {
  if (now - tSensor < 30) return;          // по одному датчику за раз
  tSensor = now;
  int i = sensorIdx;
  int d = measure(i);
  bool ok = (d >= 2 && d <= 300);
  if (ok) { good[i]++; bad[i] = 0; } else { bad[i]++; good[i] = 0; }
  if (bad[i] >= 5)  alive[i] = false;
  if (good[i] >= 3) alive[i] = true;
  dist[i] = (alive[i] && ok) ? d : 400;    // мертвий або сміття = вільно
  sensorIdx = (sensorIdx + 1) % 3;
}

void setFrame(int f) {                     // -FRAME_MAX = вліво, + = вправо
  frame = constrain(f, -FRAME_MAX, FRAME_MAX);
  int s = frame * 3 / 2;                   // тяга: серво в 1.5 раза більше за раму
  if (SERVO_INVERT) s = -s;
  servoTarget = SERVO_CENTER + s;
}

void updateServo(unsigned long now) {      // плавно, по 1°
  if (now - tServo < SERVO_STEP_MS) return;
  tServo = now;
  if (servoNow < servoTarget) servoNow++;
  else if (servoNow > servoTarget) servoNow--;
  servo.write(servoNow);
}

void updateSpeed(unsigned long now) {      // плавний розгін/гальмування
  if (now - tSpeed < 20) return;
  tSpeed = now;
  if (speedNow < speedTarget) speedNow = min(speedNow + ACCEL, speedTarget);
  else if (speedNow > speedTarget) speedNow = max(speedNow - ACCEL * 2, speedTarget);  // гальмує вдвічі швидше
  motor(speedNow);
}

void decide(unsigned long now) {
  int l = dist[LEFT], f = dist[FRONT], r = dist[RIGHT];

  // --- швидкість від переднього датчика ---
  if (f <= STOP_CM)      speedTarget = 0;
  else if (f >= SLOW_CM) speedTarget = SPEED;
  else                   speedTarget = map(f, STOP_CM, SLOW_CM, MIN_SPEED, SPEED);

  // --- об'їзд збоку ---
  if (r < AVOID_CM && r <= l) {            // справа перешкода -> вліво
    holdFrame = -FRAME_AVOID; holdUntil = now + HOLD_MS;
  } else if (l < AVOID_CM && l < r) {      // зліва перешкода -> вправо
    holdFrame = FRAME_AVOID;  holdUntil = now + HOLD_MS;
  }
  setFrame(now < holdUntil ? holdFrame : 0);
}

void setup() {
  Serial.begin(9600);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  for (int i = 0; i < 3; i++) { pinMode(TRIG[i], INPUT); pinMode(ECHO[i], INPUT); }
  pinMode(LED_BUILTIN, OUTPUT);
  motor(0);
  servo.attach(SERVO_PIN);
  servo.write(SERVO_CENTER);
  Serial.println("start in 3 s");
  for (int i = 0; i < 30; i++) { digitalWrite(LED_BUILTIN, i % 2); delay(100); }
}

void loop() {
  unsigned long now = millis();
  updateSensors(now);
  decide(now);
  updateSpeed(now);
  updateServo(now);
  digitalWrite(LED_BUILTIN, speedNow > 0);

  if (now - tPrint > 200) {
    tPrint = now;
    const char *name[3] = {"L ", "  F ", "  R "};
    for (int i = 0; i < 3; i++) {
      Serial.print(name[i]);
      if (alive[i]) Serial.print(dist[i]); else Serial.print("DEAD");
    }
    Serial.print("  speed "); Serial.print(speedNow);
    Serial.print("  frame "); Serial.println(frame);
  }
}
