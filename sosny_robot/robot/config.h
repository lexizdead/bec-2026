// config.h — ВСІ піни й налаштування тут. В інших файлах чисел не пишемо.
#pragma once

// ---------- Піни (Arduino Nano) ----------
// Мотор ОДИН: L298N OUT3/OUT4 -> канал B. Перемичку ENB на L298N зняти.
const uint8_t PIN_ENB = 6,  PIN_IN3 = 11, PIN_IN4 = 12;
const uint8_t PIN_SERVO = 9;                             // серво шарніра рами
const uint8_t PIN_START = 2;                             // кнопка на GND
const uint8_t PIN_TRIG = A2, PIN_ECHO = A3;              // передній HC-SR04 (як на schematic/t150_wiring.svg)

// ---------- Серво (ламана рама) ----------
const int  SERVO_CENTER = 90;     // рама стоїть рівно (підібрати)
const int  STEER_MAX    = 30;     // ±30°, як у Т-150К
const bool SERVO_INVERT = false;  // якщо "вліво" ламає вправо -> true

// ---------- Рух ----------
const int  SPEED_FWD   = 170;     // 0..255
const bool MOTOR_INVERT = false;  // їде назад замість вперед -> true (або поміняти дроти на OUT3/OUT4)

// ---------- Перешкода спереду, см ----------
const int STOP_CM = 20;           // ближче -> стоп
const int GO_CM   = 30;           // далі -> знову їдемо (запас, щоб не смикався на межі)

// ---------- Для avoid.ino (зараз не викликається, лишили на потім) ----------
const int SPEED_SLOW = 120;
const int SPEED_TURN = 150;
const int CLEAR_CM = 45;
const unsigned long BACK_MS      = 350;
const unsigned long EXTRA_MS     = 400;
const unsigned long STRAIGHT_MS  = 700;
const unsigned long SIDE_MAX_MS  = 2500;
const unsigned long STEER_SETTLE = 250;

const long SERIAL_BAUD = 115200;  // те саме число, що sosny.baud у .vscode/settings.json
