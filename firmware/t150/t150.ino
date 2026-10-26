// sosny — Т-150: ламана рама на SG90 (через тягу, scad/t150_joint.scad),
// три HC-SR04, кнопка Start. Після натискання їде сам; ще раз натиснути — стоп.
// Без delay: кожна задача сама дивиться на millis().
#include <Servo.h>

// ---------- Піни (Arduino Nano) ----------
const int PIN_ENA = 5, PIN_IN1 = 7,  PIN_IN2 = 8;    // задній мотор (L298N)
const int PIN_ENB = 6, PIN_IN3 = 11, PIN_IN4 = 12;   // передній мотор
const int PIN_SERVO = 9;
const int PIN_START = 2;                             // кнопка: D2 і GND
//                 лівий  передній  правий
const int TRIG[3] = { A0,    A2,      A4 };
const int ECHO[3] = { A1,    A3,      A5 };
enum { LEFT = 0, FRONT = 1, RIGHT = 2 };

// ---------- Налаштування ----------
const int SERVO_CENTER = 90;   // рама рівно (підібрати)
const int FRAME_MAX    = 20;   // рама ламається максимум на ±20° (межа в трубі Ø103, див. презентацію)
const int LINK_RATIO_X2 = 3;   // тяга: серво крутиться в 3/2 = 1.5 раза більше за раму
const bool SERVO_INVERT = false;  // якщо "вліво" ламає вправо -> true

const int SPEED_FWD  = 170;    // 0..255
const int SPEED_SLOW = 120;
const int SPEED_BACK = 130;

const int FREE_CM = 30;        // попереду вільно
const int STOP_CM = 15;        // ближче — їдемо назад

const unsigned long PING_MS   = 25;   // пауза між датчиками
const unsigned long PRINT_MS  = 200;
const unsigned long DEBOUNCE_MS = 50;

// ---------- Стан ----------
Servo steer;
int  dist[3] = { 400, 400, 400 };
int  which = 0;
bool running = false;
unsigned long lastPing = 0, lastPrint = 0, lastBtn = 0;
int  btnPrev = HIGH;

// ---------- Мотори ----------
void motor(int en, int a, int b, int spd) {
  digitalWrite(a, spd > 0);
  digitalWrite(b, spd < 0);
  analogWrite(en, abs(spd));
}
void drive(int spd) {          // обидва однаково — кермує рама
  motor(PIN_ENA, PIN_IN1, PIN_IN2, spd);
  motor(PIN_ENB, PIN_IN3, PIN_IN4, spd);
}

// ---------- Рама ----------
// frame: -FRAME_MAX (вліво) .. 0 .. +FRAME_MAX (вправо) — кут РАМИ, не серво
void setFrame(int frame) {
  frame = constrain(frame, -FRAME_MAX, FRAME_MAX);
  int servo = frame * LINK_RATIO_X2 / 2;     // ±20° рами -> ±30° серво
  if (SERVO_INVERT) servo = -servo;
  steer.write(SERVO_CENTER + servo);
}

// ---------- Датчики ----------
int measure(int i) {
  digitalWrite(TRIG[i], LOW);  delayMicroseconds(2);
  digitalWrite(TRIG[i], HIGH); delayMicroseconds(10);
  digitalWrite(TRIG[i], LOW);
  long us = pulseIn(ECHO[i], HIGH, 12000);   // макс. 12 мс (~2 м)
  if (us == 0) return 400;
  return us / 58;
}

// ---------- Задачі ----------
void taskButton(unsigned long now) {          // натиснули -> старт/стоп
  int b = digitalRead(PIN_START);
  if (b != btnPrev && now - lastBtn > DEBOUNCE_MS) {
    lastBtn = now;
    btnPrev = b;
    if (b == LOW) running = !running;
  }
}

void taskSensors(unsigned long now) {         // один датчик за раз
  if (now - lastPing < PING_MS) return;
  lastPing = now;
  dist[which] = measure(which);
  which = (which + 1) % 3;
}

void taskDrive() {
  if (!running) { drive(0); setFrame(0); return; }

  int sideFrame = (dist[LEFT] > dist[RIGHT]) ? -FRAME_MAX : FRAME_MAX;

  if (dist[FRONT] > FREE_CM) {                // вільно — прямо
    setFrame(0);
    drive(SPEED_FWD);
  } else if (dist[FRONT] > STOP_CM) {         // близько — повільно в бік, де більше місця
    setFrame(sideFrame);
    drive(SPEED_SLOW);
  } else {                                     // впритул — назад з рамою в інший бік
    setFrame(-sideFrame);
    drive(-SPEED_BACK);
  }
}

void taskPrint(unsigned long now) {
  if (now - lastPrint < PRINT_MS) return;
  lastPrint = now;
  Serial.print(running ? "RUN  " : "STOP ");
  Serial.print(dist[LEFT]);  Serial.print("  ");
  Serial.print(dist[FRONT]); Serial.print("  ");
  Serial.println(dist[RIGHT]);
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(9600);
  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT); pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENA, OUTPUT); pinMode(PIN_ENB, OUTPUT);
  pinMode(PIN_START, INPUT_PULLUP);
  for (int i = 0; i < 3; i++) { pinMode(TRIG[i], OUTPUT); pinMode(ECHO[i], INPUT); }
  drive(0);
  steer.attach(PIN_SERVO);
  setFrame(0);
}

void loop() {
  unsigned long now = millis();
  taskButton(now);
  taskSensors(now);
  taskDrive();
  taskPrint(now);
}
