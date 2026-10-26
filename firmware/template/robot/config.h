// ======================================================================
//  config.h — УСІ налаштування в одному місці (піни, пороги, швидкості)
//  Шаблон прошивки команди sosny. Новий день = нове завдання:
//  спочатку правимо ТІЛЬКИ цей файл, потім логіку станів у robot.ino.
// ======================================================================
#pragma once
#include <Arduino.h>

#define SETTINGS_VERSION 1      // збільшіть, якщо змінили дефолти нижче

// ---------- Піни Arduino Nano (перевірте зі схемою Андрія/Віталія!) ----------
#define PIN_ENA     5    // ШІМ лівого мотора (L298N, перемичку ENA зняти)
#define PIN_IN1     7
#define PIN_IN2     8
#define PIN_ENB     6    // ШІМ правого мотора
#define PIN_IN3     11
#define PIN_IN4     12
#define PIN_SERVO   9    // серво (будь-який механізм)
#define PIN_BUTTON  4    // кнопка «Старт» на GND (D2/D3 лишаємо під енкодери)
#define PIN_ENC_L   2    // INT0
#define PIN_ENC_R   3    // INT1
#define PIN_LED     LED_BUILTIN

// ---------- Аналогові датчики (TCRT5000 тощо) ----------
// Додати датчик = дописати пін у масив. Індекси — константи нижче.
#define N_SENSORS 2
static const uint8_t SENSOR_PINS[N_SENSORS] = {A0, A3};
enum SensorId : uint8_t { S_LINE = 0, S_MARK = 1 };
// після калібрування: 0 = біле, 1000 = чорне
#define BLACK_THR   600
#define WHITE_THR   150
#define CALIB_MS    5000

// ---------- Мотори ----------
#define MOTOR_L_INVERT 0
#define MOTOR_R_INVERT 0
#define PWM_MAX        255

// ---------- Енкодери ----------
#define ENC_COUNT   0          // 0 = немає, 1 = лівий, 2 = обидва
#define ENC_SLOTS   20
#define WHEEL_D_MM  65.0
#define MM_PER_TICK (PI * WHEEL_D_MM / (2.0 * ENC_SLOTS))

// ---------- Керування (дефолти, міняються через Serial і зберігаються в EEPROM) ----------
#define CONTROL_DT_US  4000    // цикл 250 Гц
#define DEF_KP   0.25
#define DEF_KI   0.0
#define DEF_KD   2.0
#define DEF_BASE_PWM 140
#define LINE_SETPOINT 500      // тримати край лінії (50 % чорного)

// ---------- Серво ----------
#define SERVO_HOME_DEG 0

// ---------- Безпека ----------
#define START_DELAY_MS 1000
#define RUN_TIMEOUT_MS 180000UL
#define SERIAL_BAUD    115200
