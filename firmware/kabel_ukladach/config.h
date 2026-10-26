// ======================================================================
//  config.h — УСІ налаштування робота в одному місці
//  Робот «Швидкісний кабель-укладач», BEST Engineering Competition
//
//  Значення з позначкою (EEPROM) можна міняти без перепрошивки через
//  Serial-меню (команди P, I, D, B, Z, A, O, T, потім w — зберегти).
//  Якщо змінили дефолти тут і хочете, щоб вони застосувались,
//  збільште SETTINGS_VERSION або дайте в Serial команду x.
// ======================================================================
#pragma once
#include <Arduino.h>

#define SETTINGS_VERSION 1

// ---------- Піни Arduino Nano ----------
#define PIN_ENA     5    // ШІМ лівого мотора (L298N ENA, перемичку зняти)
#define PIN_IN1     7
#define PIN_IN2     8
#define PIN_ENB     6    // ШІМ правого мотора (L298N ENB, перемичку зняти)
#define PIN_IN3     11
#define PIN_IN4     12
#define PIN_SERVO   9    // SG90 револьверного магазину
#define PIN_BUTTON  4    // кнопка «Старт» на GND (ПЕРЕНЕСЕНО з D2!)
#define PIN_ENC_L   2    // енкодер лівого колеса (INT0)
#define PIN_ENC_R   3    // енкодер правого колеса (INT1)
#define PIN_LINE    A0   // TCRT5000 лінії, вихід AO
#define PIN_MARK    A3   // TCRT5000 міток, вихід AO
#define PIN_LED     LED_BUILTIN  // D13, світлодіод на платі

// ---------- Мотори ----------
#define MOTOR_L_INVERT 0   // 1, якщо лівий крутиться назад
#define MOTOR_R_INVERT 0   // 1, якщо правий крутиться назад
#define BRAKE_MS       120 // активне гальмо L298N перед скиданням

// ---------- Енкодери (HC-020K / LM393 + диск 20 щілин) ----------
#define ENC_COUNT       2      // 0 = немає (режим за часом), 1 = лише лівий, 2 = обидва
#define ENC_SLOTS       20     // щілин у диску
#define ENC_DEBOUNCE_US 800    // ігнорувати фронти, ближчі за це (брязкіт LM393)
#define WHEEL_D_MM      65.0   // діаметр ведучого колеса (поміряти!)
// CHANGE рахує обидва фронти → 2 тики на щілину. ~5.1 мм на тик.
#define MM_PER_TICK     (PI * WHEEL_D_MM / (2.0 * ENC_SLOTS))

// ---------- Датчики ----------
// Нормалізовані значення: 0 = біле, 1000 = чорне (після калібрування)
#define LINE_ON_LEFT_EDGE 1    // 1 = датчик лінії над ЛІВИМ краєм смужки (як у sensor_arm.scad), 0 = над правим
#define LINE_SETPOINT     500  // PID тримає 50 % чорного
#define MARK_THR          600  // датчик міток «бачить чорне»
#define MARK_REQUIRE_LINE 1    // мітка перетинає лінію → датчик лінії теж має бути майже чорним
#define LINE_CONFIRM_THR  750
#define MARK_MIN_SAMPLES  3    // скільки циклів поспіль (по 4 мс) має бути чорне
#define LOST_THR          150  // нижче — лінії немає (біле)

// дефолти калібрування, якщо EEPROM порожній (сирі значення analogRead)
#define DEF_LINE_WHITE 100
#define DEF_LINE_BLACK 900
#define DEF_MARK_WHITE 100
#define DEF_MARK_BLACK 900
#define CALIB_MS       6000    // тривалість калібрування

// ---------- PID і швидкості (EEPROM) ----------
#define CONTROL_DT_US  4000    // цикл керування 250 Гц
#define DEF_KP         0.25
#define DEF_KI         0.0
#define DEF_KD         2.0
#define I_LIMIT        20000.0
#define DEF_BASE_PWM   140     // крейсерська
#define DEF_SLOW_PWM   110     // біля очікуваної мітки
#define DEF_APPROACH_PWM 100   // доїзд від мітки до точки скидання
#define PWM_MAX        255

// ---------- Мітки й скидання ----------
#define N_DOWELS          9
#define MARK_SPACING_MM   500   // крок міток за регламентом
#define FIRST_MARK_MM     500   // де очікуємо першу мітку від старту
#define SLOW_ZONE_MM      120   // за скільки мм до очікуваної мітки сповільнюватись
#define MARK_MIN_GAP_MM   300   // нову мітку не приймаємо ближче за це до попередньої
#define MARK_MIN_GAP_MS   1500  // те саме без енкодерів
#define FORCE_DROP_ON_MISS 1    // пропустив мітку → скинути там, де вона мала бути (лише з енкодером)
#define MISS_WINDOW_MM    80    // скільки чекати мітку після очікуваної позиції
// Відстань уздовж робота від датчика міток до точки, куди падає шкант.
// За chassis.scad: датчик x = +145, щілина x = −35 → 180 мм. ПОМІРЯЙТЕ на зібраному роботі.
#define DEF_DROP_OFFSET_MM 180  // (EEPROM)
#define BRAKE_COMP_MM      5    // гальмівний шлях: зупиняємось раніше на стільки
#define DEF_DROP_DELAY_MS  1200 // (EEPROM) без енкодерів: скільки їхати від мітки до скидання

// ---------- Револьверний магазин SG90 ----------
// Кут для кожної позиції: [0] = старт (усі камери повні), [k] = після k-го скидання.
// Задайте під реальну конструкцію барабана.
static const uint8_t MAG_ANGLES[N_DOWELS + 1] = {0, 20, 40, 60, 80, 100, 120, 140, 160, 180};
#define MAG_PULSE_MIN    500    // мкс, для повного ходу SG90 0–180°
#define MAG_PULSE_MAX    2400
#define MAG_STEP_DEG     2      // плавний поворот: крок
#define MAG_STEP_MS      8      //                  пауза між кроками
#define MAG_DROP_WAIT_MS 400    // чекати, поки шкант випаде

// ---------- Старт, фініш, безпека ----------
#define START_DELAY_MS   1000   // після натискання «Старт»
#define FINISH_LOST_MM   80     // після 9-го шканта: лінії немає стільки мм → стоп
#define FINISH_LOST_MS   700    //   те саме без енкодерів
#define LOST_STOP_MM     400    // до 9-го шканта: лінію втрачено так довго → аварійний стоп
#define LOST_STOP_MS     2500
#define RUN_TIMEOUT_MS   180000 // 3 хв — і стоп у будь-якому разі

#define SERIAL_BAUD 115200
