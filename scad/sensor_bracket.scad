// ======================================================================
//  КРОНШТЕЙН ДАТЧИКІВ ЛІНІЇ (TCRT5000) · sosny · BEST EC Day 2 (TD)
//
//  Форма «С»: зверху полиця (як у motor_holder / free_wheel — ті самі
//  прорізи під стяжки, крок ламелей 20 мм), ззаду стінка, знизу площадка.
//  Модулі TCRT5000 лежать на площадці, сенсор дивиться вниз крізь вікно.
//  Розклад сенсорів — з плану Віталія (claude/day2-plan.md):
//  L / C / R з кроком 15 мм + 4-й (поперечні лінії) на 35 мм збоку.
//  Вікна: −35, −15, 0, +15, +35 — 4-й ставиться в будь-яке крайнє вікно. Модуль кріпиться гвинтом М3
//  у поздовжній паз (підходить під різні модулі) або стяжкою.
//
//  ВИСОТА (рахується сама з параметрів нижче):
//    рама над підлогою = wheel_d/2 + axle_to_frame = 33 + 14.7 = 47.7 мм
//    верх площадки     = sensor_gap + tcrt_h       = 6 + 7      = 13 мм
//    кінчик сенсора    = 6 мм над підлогою (план: 5–8 мм)
//  !!! ЗАМІРЯТИ tcrt_h: від плати модуля до кінчика сенсора !!!
//
//  Друк: PLA 0.28, 3 стінки, 25 %, БЕЗ підтримок, стінкою на стіл
//  (print_orient = true — STL уже так повернутий).
// ======================================================================

print_orient = true;

// --- висоти ---
wheel_d       = 66;    // колесо з шиною (заміряно)
axle_to_frame = 14.7;  // від осі до низу рами (motor_holder / free_wheel)
tcrt_h        = 7;     // від плати модуля до кінчика сенсора — ЗАМІРЯТИ
sensor_gap    = 6;     // кінчик сенсора → підлога (план Віталія: 5–8 мм)
plat_t        = 3;     // товщина площадки

frame_z = wheel_d/2 + axle_to_frame;     // 47.7
plat_top = sensor_gap + tcrt_h;          // 13
plat_bot = plat_top - plat_t;            // 10

// --- форма ---
wid      = 84;     // ширина площадки (Y) ±42; стінка = ширина полиці,
                   // щоб не зачепити редуктори моторів збоку
wall_t   = 4;
top_rib_y   = [-25, 25]; // вертикальні ребра жорсткості зверху — між модулями ±15 і ±35
top_rib_w   = 2;
top_rib_h   = 6;
top_rib_len = 56;     // від стінки до смуги з вікнами — зв'язує «драбинку»
plat_len = 66;     // площадка вперед від задньої грані стінки
flange_len = 36;
flange_w = 52;
flange_t = 3;

// --- вікна під сенсори ---
win_y    = [-35, -15, 0, 15, 35];
win_x    = plat_len - 8;   // центр вікон від задньої грані (58 мм)
win_dx   = 7;              // вздовж руху
win_dy   = 11;             // поперек
m3_w     = 3.4;            // паз під М3
m3_from  = 8;              // паз від центру вікна назад: від…
m3_to    = 22;             // …до

// --- прорізи під стяжки (як в інших тримачах) ---
slot_l = 4.4; slot_w = 2.2; slat_pitch = 20; slat_w = 5; out_y = 25.4;
gus = 12;  // косинки
lite  = true;  // площадка-«драбинка»: смуга з вікнами + ребра під модулі (на ~40 % менше пластику)
rear_w = 14;   // смуга біля стінки
rib_w  = 9;    // ребро під модуль (паз М3 посередині)

module tri(a, b) {  // косинка в площині X-Z, товщина 4 по Y
    rotate([90, 0, 0]) linear_extrude(4, center = true)
        polygon([[0, 0], [a, 0], [0, b]]);
}

module bracket() {
    difference() {
        union() {
            translate([0, -flange_w/2, plat_bot]) cube([wall_t, flange_w, frame_z - plat_bot]);   // стінка
            translate([0, -flange_w/2, frame_z - flange_t]) cube([flange_len, flange_w, flange_t]); // полиця
            if (!lite) translate([0, -wid/2, plat_bot]) cube([plat_len, wid, plat_t]);   // суцільна площадка
            else {                                                                      // «драбинка» — швидше друкується
                translate([0, -wid/2, plat_bot]) cube([rear_w, wid, plat_t]);                         // біля стінки
                translate([win_x - win_dx/2 - 3, -wid/2, plat_bot]) cube([plat_len - (win_x - win_dx/2 - 3), wid, plat_t]); // смуга з вікнами
                for (y = win_y) translate([0, y - rib_w/2, plat_bot]) cube([plat_len, rib_w, plat_t]);  // ребро під кожен модуль
            }
            for (ry = top_rib_y)
                translate([0, ry - top_rib_w/2, plat_bot]) cube([top_rib_len, top_rib_w, plat_t + top_rib_h]);
            for (sy = [-1, 1]) {
                translate([wall_t, sy*(flange_w/2 - 2), frame_z - flange_t]) mirror([0, 0, 1]) tri(gus, gus);
                translate([wall_t, sy*(flange_w/2 - 2), plat_top]) tri(gus, gus);
            }
        }
        // вікна + пази М3
        for (y = win_y) {
            translate([win_x - win_dx/2, y - win_dy/2, plat_bot - 1]) cube([win_dx, win_dy, plat_t + 2]);
            hull() for (dx = [m3_from, m3_to])
                translate([win_x - dx, y, plat_bot - 1]) cylinder(d = m3_w, h = plat_t + 2, $fn = 24);
        }
        // прорізи під стяжки в полиці
        xc0 = wall_t + (flange_len - wall_t - slat_pitch)/2;
        for (xc = [xc0, xc0 + slat_pitch], dx = [-1, 1], sy = [-1, 1])
            translate([xc + dx*(slat_w/2 + 0.5 + slot_w/2) - slot_w/2,
                       sy*(out_y/2 + 7) - slot_l/2, frame_z - flange_t - 1])
                cube([slot_w, slot_l, flange_t + 2]);
    }
}

if (print_orient) rotate([0, -90, 0]) translate([0, 0, -plat_bot]) bracket();
else bracket();
