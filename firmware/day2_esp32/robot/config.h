// ======================================================================
//  config.h — УСІ налаштування в одному місці (піни, пороги, швидкості)
//  sosny · BEST EC Day 2 (TD) · ESP32 · шаблон Ліди, адаптований під день 2
//
//  Залізо — як домовились Андрій і Віталій (claude/day2-plan.md,
//  schematic/day2_wiring.svg), механіка — scad/*.scad:
//    • танковий хід: M1 ліве, M2 праве (TT у motor_holder, ламелі №3+№4),
//      ззаду 2 вільні колеса (№0+№1); база 98 мм
//    • 4× TCRT5000 у sensor_bracket: L / C / R з кроком 15 мм + 4-й на 35 мм
//      збоку; кінчики 6 мм над підлогою; ~40 мм ПЕРЕД ведучою віссю
//    • HC-SR04 зверху на передньому краї рами
//  Міняєте пін — скажіть Андрію й Віталію і виправте схему.
// ======================================================================
#pragma once
#include <Arduino.h>

#define SIM 0                   // 1 = Wokwi (без Wi-Fi, команди через Serial)

// ---------- Піни ESP32 (= day2_wiring.svg) ----------
#define PIN_ENA    25           // ШІМ M1 (ліве). Перемички ENA/ENB на L298N ЗНЯТИ
#define PIN_IN1    26
#define PIN_IN2    27
#define PIN_IN3    16           // на WROVER 16/17 зайняті → 21 і 5
#define PIN_IN4    17
#define PIN_ENB    18           // ШІМ M2 (праве)
#define PIN_TRIG   15           // HC-SR04 (D15; перенесено з 19/23 о 17:00, як розпаяно)
#define PIN_ECHO   39           // VN, тільки вхід; через дільник 1k/2k (5 В → 3.3 В)
#define PIN_SERVO  22           // SG90 кузова
#define PIN_BUZZ   4            // активний зумер через 2N2222
#define PIN_START  13           // кнопки на GND, внутрішня підтяжка
#define PIN_STOP   14
#define PIN_LED    2            // вбудований світлодіод
#define PIN_LED_GO   21         // зовнішній зелений «їду» (через 220 Ом на GND)
#define PIN_LED_STOP 5          // зовнішній червоний «стою» (через 220 Ом на GND)
#define PIN_TURN_L 12           // жовтий «поворот ліворуч» (220 Ом на GND!)
#define PIN_TURN_R 23           // жовтий «поворот праворуч» (220 Ом на GND) — 15 тепер TRIG
                                // 12 — strapping-пін: світлодіод ТІЛЬКИ на GND, без підтяжки до 3.3 В
#define PIN_BATT   36           // дільник 20k/10k → ×3

// ---------- Датчики TCRT5000 (живлення 3.3 В!, лише ADC1) ----------
#define N_SENSORS 4
static const uint8_t SENSOR_PINS[N_SENSORS] = {32, 33, 34, 35};
enum SensorId : uint8_t { S_L = 0, S_C = 1, S_R = 2, S_X = 3 };   // S_X — 4-й, поперечні лінії
#define ON_LINE    500          // після калібрування 0 = біле, 1000 = чорне
#define CALIB_MS   5000

// ---------- Мотори ----------
#define MOTOR_L_INVERT 0        // їде назад — поставте 1
#define MOTOR_R_INVERT 0
#define PWM_FREQ   1000
#define PWM_LIMIT  150          // ОБМЕЖЕННЯ швидкості (~60 %) — вимога безпеки

// ---------- Безпека (HC-SR04) ----------
#define SLOW_CM    35           // ближче → швидкість ×0.4 і переривчастий зумер
#define STOP_CM    15           // ближче → аварійна зупинка
#define SLOW_K     0.4f
#define RESUME_MS  1000         // шлях вільний стільки → їде далі сам

// ---------- Керування ----------
#define CONTROL_MS     10       // цикл 100 Гц
#define DIST_MS        60       // опитування HC-SR04
#define TELEM_MS       500      // телеметрія
#define CROSS_LOCK_MS  300      // після поперечної лінії ігнор
#if SIM
#define CROSS_CONFIRM_MS 3000   // у Wokwi повзунки рухаємо по одному — даємо 3 с
#else
#define CROSS_CONFIRM_MS 200    // після смуги лінія має йти далі під C, інакше це кут
#endif
#define LOST_MS        1500     // скільки шукати лінію розворотом
#define RUN_TIMEOUT_MS 300000UL
#define TURN_POS       0.35f   // |положення лінії| більше → вмикаємо поворотник
#define TURN_HOLD_MS   600     // тримати поворотник після повороту (без мерехтіння)
#define TURN_HALF_MS   350     // півперіоду блимання і «тік-так»

// ---------- Серво кузова ----------
#define SERVO_HOME 0
#define SERVO_DUMP 110

// ---------- Мережа ----------
#define WIFI_SSID  "sosny-robot"
#define WIFI_PASS  "sosny2026"  // ≥ 8 символів
#define WIFI_CH    6            // канал 1–13; на хакатоні багато ESP32 на каналі 1 → беремо 6 або 11
#define SERIAL_BAUD 115200

// ---------- Дефолти (міняються з Serial / веб і зберігаються в пам'яті ESP32) ----------
#define DEF_KP       70.0f
#define DEF_KD       40.0f
#define DEF_BASE     120
#define DEF_PIVOT    110
#define DEF_TARGET   3          // зупинитись на N-й поперечній лінії
#define DEF_MM_S     250        // швидкість при BASE, мм/с — ЗАМІРЯТИ (для «пройдено, см»)
#define DEF_BYPASS   1          // 1 = об'їжджати перешкоду на лінії, 0 = тільки стоп
#define DEF_BP_SIDE  1          // +1 праворуч, −1 ліворуч
// об'їзд за часом (енкодерів нема) — підібрати на трасі під розмір коробки
#define DEF_WAIT_MS   3000      // стоїть довше → це не людина, об'їжджаємо
#define DEF_T_TURN    450       // відвернути від лінії ~60–90°
#define DEF_ARC_IN    60        // дуга: внутрішнє колесо
#define DEF_ARC_OUT   150       //       зовнішнє колесо
#define DEF_T_ARC_MIN 900       // спершу відʼїхати, потім шукати лінію
#define DEF_T_ARC_MAX 6000      // не знайшли лінію → LOST
#define DEF_T_ALIGN   1200      // доворот на лінію
