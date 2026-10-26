// sosny — Т-150, повна логіка. 1 мотор (L298N OUT3/OUT4), серво рами D9, 3 x HC-SR04.
// Лівий A0/A1, передній A2/A3, правий A4/A5. Без кнопки: старт через 3 с.
//
// * Датчик не працює (нема відлуння / сміття 5 разів поспіль) -> вважається "мертвим" і
//   ігнорується: робот поводиться так, ніби там вільно, тобто просто їде вперед.
//   Ожив (3 нормальні виміри поспіль) -> знову враховується.
//   Плата не падає навіть при кривій проводці: TRIG виходом тільки на 12 мкс.
// * Серво їде до цілі плавно, по 1° (SERVO_STEP_MS).
// * Вільно -> їде прямо; стіна збоку ближче SIDE_CM -> трохи ламає раму від неї.
// * Спереду ближче SLOW_CM -> повільно, рама в бік, де більше місця.
// * Спереду ближче STOP_CM -> назад BACK_MS з рамою в інший бік, потім знову вперед.
// Світлодіод L: блимає = старт через 3 с, горить = їде вперед, не горить = назад/стоп.
#include <Servo.h>

// ---- піни ----
const int ENB = 6, IN3 = 11, IN4 = 12;
const int SERVO_PIN = 9;
const int TRIG[3] = {A0, A2, A4};   // лівий, передній, правий
const int ECHO[3] = {A1, A3, A5};
enum { LEFT = 0, FRONT = 1, RIGHT = 2 };

// ---- налаштування ----
const int  SPEED = 170, SPEED_SLOW = 120, SPEED_BACK = 140;   // 0..255
const bool MOTOR_INVERT = false;   // їде назад замість вперед -> true
const int  STOP_CM = 15, SLOW_CM = 35, SIDE_CM = 15;
const unsigned long BACK_MS = 700;

const int  SERVO_CENTER = 90;      // рама рівно (підібрати 85..95)
const int  FRAME_MAX = 30;         // рама ±30°
const int  FRAME_SIDE = 12;        // підрулювання від бічної стіни
const bool SERVO_INVERT = false;   // ламає не в той бік -> true
const unsigned long SERVO_STEP_MS = 12;   // більше = плавніше/повільніше

// ---- стан ----
Servo servo;
int dist[3] = {400, 400, 400};     // для логіки: мертвий датчик = 400 (вільно)
int bad[3] = {0, 0, 0}, good[3] = {0, 0, 0};
bool alive[3] = {true, true, true};
const int FREE = 400;
int sensorIdx = 0;
unsigned long tSensor = 0, tServo = 0, tPrint = 0, backUntil = 0;
int servoNow = SERVO_CENTER, servoTarget = SERVO_CENTER, frameTarget = 0;
bool backing = false;

void motor(int spd) {
  if (MOTOR_INVERT) spd = -spd;
  digitalWrite(IN3, spd > 0);
  digitalWrite(IN4, spd < 0);
  analogWrite(ENB, abs(spd));
}

// TRIG — виходом тільки на час імпульсу, решту часу вхід (безпечно при кривій проводці)
int measure(int i) {
  pinMode(TRIG[i], OUTPUT);
  digitalWrite(TRIG[i], LOW);  delayMicroseconds(2);
  digitalWrite(TRIG[i], HIGH); delayMicroseconds(10);
  digitalWrite(TRIG[i], LOW);
  pinMode(TRIG[i], INPUT);
  long us = pulseIn(ECHO[i], HIGH, 20000);
  return us ? us / 58 : -1;        // -1 = нема відлуння
}

void setFrame(int frame) {        // -FRAME_MAX = вліво, +FRAME_MAX = вправо
  frame = constrain(frame, -FRAME_MAX, FRAME_MAX);
  int s = frame * 3 / 2;           // тяга: серво крутиться в 1.5 раза більше за раму
  if (SERVO_INVERT) s = -s;
  frameTarget = frame;
  servoTarget = SERVO_CENTER + s;
}

void updateServo(unsigned long now) {
  if (now - tServo < SERVO_STEP_MS) return;
  tServo = now;
  if (servoNow < servoTarget) servoNow++;
  else if (servoNow > servoTarget) servoNow--;
  servo.write(servoNow);
}

void updateSensors(unsigned long now) {
  if (now - tSensor < 30) return;   // по одному датчику за раз, щоб не ловили чуже відлуння
  tSensor = now;
  int i = sensorIdx;
  int d = measure(i);
  bool ok = (d >= 2 && d <= 300);          // 2..300 см — схоже на правду
  if (ok) { good[i]++; bad[i] = 0; } else { bad[i]++; good[i] = 0; }
  if (bad[i] >= 5)  alive[i] = false;      // 5 поганих поспіль -> мертвий
  if (good[i] >= 3) alive[i] = true;       // 3 нормальні поспіль -> живий
  if (!alive[i]) dist[i] = FREE;           // мертвий -> ігноруємо (вільно)
  else if (ok)   dist[i] = d;
  else           dist[i] = FREE;           // один поганий вимір -> теж вважаємо вільно
  sensorIdx = (sensorIdx + 1) % 3;
}

void decide(unsigned long now) {
  int l = dist[LEFT], f = dist[FRONT], r = dist[RIGHT];
  int openSide = (l > r) ? -FRAME_MAX : FRAME_MAX;   // куди більше місця

  if (backing) {
    if (now < backUntil) { motor(-SPEED_BACK); return; }
    backing = false;
  }
  if (f < STOP_CM) {                       // впритул -> назад, рама в інший бік
    backing = true;
    backUntil = now + BACK_MS;
    setFrame(-openSide);
    motor(-SPEED_BACK);
  } else if (f < SLOW_CM) {                // близько -> повільно в бік, де місце
    setFrame(openSide);
    motor(SPEED_SLOW);
  } else {                                 // вільно -> прямо, відштовхуємось від бічних стін
    if (l < SIDE_CM && r >= SIDE_CM)      setFrame(FRAME_SIDE);
    else if (r < SIDE_CM && l >= SIDE_CM) setFrame(-FRAME_SIDE);
    else                                  setFrame(0);
    motor(SPEED);
  }
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
  updateServo(now);
  decide(now);
  digitalWrite(LED_BUILTIN, !backing);

  if (now - tPrint > 200) {
    tPrint = now;
    const char *name[3] = {"L ", "  F ", "  R "};
    for (int i = 0; i < 3; i++) {
      Serial.print(name[i]);
      if (alive[i]) Serial.print(dist[i]); else Serial.print("DEAD");
    }
    Serial.print("  frame "); Serial.print(frameTarget);
    Serial.println(backing ? "  BACK" : "");
  }
}
