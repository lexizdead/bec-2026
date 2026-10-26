# sosny · BEST Engineering Competition 2026

Три автономні роботи, які команда **sosny** зробила за три дні категорії Team Design (25–27.10.2026, Львів).
Щодня було нове завдання, і за один день ми робили робочий прототип: механіку, електроніку й прошивку.

Команда: Андрій (капітан, пайка), Віталій (пайка, код), Ліда (код, 3D), Нікіта (3D).

## Роботи

| День | Робот | Плата | Що вміє |
|---|---|---|---|
| 1 | Швидкісний кабель-укладач | Arduino Nano | їде по лінії (PID), розмотує ≥ 7 м кабелю, скидає 9 дюбелів на мітках кожні 500 мм |
| 2 | Автономний транспортний IoT-робот | ESP32 | їде по лінії без GPS, рахує поперечні мітки, об'їжджає перешкоди, віддає телеметрію через Wi-Fi |
| 3 | Шарнірний розвідник за схемою Т-150 | Arduino Nano | проходить трубу 110 мм, кермує шарніром рами на сервоприводі, об'їжджає перешкоди за 3 сонарами |

## Де що лежить

| День | Прошивка | 3D-моделі (`scad/`) |
|---|---|---|
| 1 | `firmware/kabel_ukladach/`, `firmware/line_test/`, `firmware/Makefile` | `chassis`, `sensor_arm`, `spool`, `revolver`, `magazine` (стара версія), `plate` |
| 2 | `firmware/day2_esp32/` (основна), `firmware/day2_robot/` (Wokwi), `firmware/motor_test/`, `firmware/drive_test/` | `board_plate`, `motor_holder`, `free_wheel`, `rear_wheel`, `sensor_bracket`, `wheel_axle_round` |
| 3 | `firmware/t150/`, `firmware/articulated_avoid/`, `sosny_robot/` (спільний проєкт Linux + Windows) | `t150_joint`, `articulation_joint`, `axle_holder`, `wheel_axle_double` |
| усі | `firmware/template/` — шаблон прошивки, з якого починали кожен день | |

## Інструменти
Arduino C++, arduino-cli, Make, bash, Wokwi (симуляція ESP32), OpenSCAD, Tinkercad, Linux (Arch) і Windows.

## Як зібрати
- Arduino Nano: `cd firmware && make setup && make build`
- ESP32 (день 2): `cd firmware/day2_esp32 && make setup && make build`
- Т-150 на Linux або Windows: див. `sosny_robot/README.md`

Wi-Fi `sosny-robot` / `sosny2026` у прошивці дня 2 — це точка доступу, яку піднімає сам робот для телеметрії, а не чийсь особистий пароль.

## Як ми працювали
Код і моделі ми писали разом з ШІ-асистентом (Claude): ставили задачу, перевіряли на залізі й у симуляторі, правили за результатами тестів.
