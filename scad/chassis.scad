// ======================================================================
//  ПРОСТИЙ КАРКАС: плоска дека-решітка (на неї все клеїться термоклеєм
//  або стяжками) + передня качалка з 2 моторами TT + 2 задні вільні колеса
//  Робот «Швидкісний кабель-укладач», BEST Engineering Competition
//
//  ПРИВІД ПЕРЕДНІЙ. Обидва ведучі мотори в одній передній балці (качалці),
//  яка гойдається на поздовжній осі ±7° (упори на деці) — це підвіска з
//  регламенту. Задні колеса вільні на друкованих напіввісях.
//
//  ЕНКОДЕРИ. Диск HC-020K (Ø ~25 мм) сідає на внутрішній кінець валу TT,
//  у качалці під нього проріз (enc_slot = 1). Модуль LM393 — термоклеєм.
//
//  ДЕ ЩО СТОЇТЬ (узгоджено з прошивкою, config.h: DEF_DROP_OFFSET_MM 180):
//    датчик міток   x = +145 (шарнір плеча x = +85, плече 60 мм)
//    отвір скидання x = −35, по центру (дека має отвір Ø16)
//    → від датчика міток до точки падіння 180 мм.
//    Револьвер: центр (x −35, y +40), його отвір дивиться на −Y,
//    тобто точно над отвором деки (revolver.scad, hole_ang = −90).
//    Котушка: ззаду праворуч, центр (x −68, y −32), кабель назад.
//    Nano, L298N, DC-DC, акумулятор: на передній частині деки над качалкою.
//
//  Деталі:
//    deck       — дека-решітка з вушками шарніра, упорами, кронштейнами
//                 задніх коліс, отвором скидання і отворами під плече датчиків
//    rocker     — качалка з гніздами під 2 мотори TT
//    pivot_pin  — вісь качалки 6 мм (фіксується філаментом 1.75)
//    rear_stub  — напіввісь заднього колеса (друкувати 2 шт.)
//    motor_test — шматок гнізда мотора: спершу перевірте посадку
//
//  Друк швидкий: шар 0.28, 2 периметри, 15 %, без підтримок.
//  Координати: X вперед, Y вліво, Z вгору, Z = 0 це траса.
// ======================================================================

part = "all";   // [all, deck, rocker, pivot_pin, rear_stub, motor_test]

// ---------- Колеса (стандартні до TT) ----------
wheel_d  = 65;
wheel_w  = 26;

// ---------- Мотор TT (стандартний жовтий, ПОМІРЯЙТЕ свій) ----------
tt_t     = 18.8;   // товщина вздовж валу
tt_h     = 22.5;   // висота редуктора
tt_bl    = 37;     // довжина корпусу редуктора
tt_l     = 65;     // повна довжина разом з двигуном
tt_sx    = 11.5;   // від переднього торця редуктора до валу
tt_boss_d = 10;    // отвір під внутрішній кінець валу
mc       = 0.4;    // зазор гнізда

// ---------- Енкодери ----------
enc_slot   = 1;    // 1 = проріз під диск енкодера, 0 = без
enc_disc_d = 25;
enc_slot_w = 8;

// ---------- Компонування ----------
x_fa     = 55;     // передня (ведуча) вісь
x_ra     = -95;    // задня вісь (база 150 мм)
motor_y  = 45;     // внутрішня грань мотора від центру
wheel_gap = 1;
z_deck   = 56;     // низ деки над трасою (frame_h у sensor_arm.scad і spool.scad)
deck_t   = 3;
deck_x0  = -112;
deck_x1  = 97;
hw_front = 56;     // півширина деки над качалкою
hw_rear  = 64;     // півширина деки ззаду
x_split  = -30;

// ---------- Решітка деки ----------
win      = 18;     // вікно
web      = 6;      // перемичка між вікнами (сюди клеїти)
border   = 6;      // суцільний край

// ---------- Підвіска ----------
z_p      = 49;     // висота осі качання
pivot_d  = 6;

// ---------- Задні колеса ----------
stub_d   = 6;
br_t     = 7;

// ---------- Отвір скидання ----------
x_drop   = -35;
drop_d   = 16;

// ---------- Кріплення плеча датчиків (як у sensor_arm.scad) ----------
hinge_x  = 85;
mark_y   = 40;
mark_side = 1;

fil_d    = 2.0;
$fn = 48;

// ======================= похідні величини =============================
R      = wheel_d / 2;
zs     = R;
gf     = x_fa + tt_sx;
gb     = gf - tt_bl;
x_can  = gf - tt_l;
y_out  = motor_y + tt_t;
y_wf   = y_out + wheel_gap;
y_wr   = hw_rear + 2;
iw     = 2.5;
z_fl_top = zs - tt_h/2 - mc/2;
z_fl_bot = z_fl_top - 2.5;
z_rf_bot = zs + tt_h/2 + mc/2;
z_rf_top = z_rf_bot + 1.6;
lx0    = gb - 20;
lx1    = gf - 8;
pin_r  = pivot_d/2 + 3;
lite_hw = enc_slot ? 22 : motor_y - 14;
pw_    = win + web;

width  = 2*max(y_wf, y_wr) + 2*wheel_w;
echo(str("Ширина по колесах ", width, " мм (ліміт 200)"));
echo(str("Довжина від заднього колеса до передка деки ", deck_x1 - (x_ra - R), " мм"));
echo(str("Від датчика міток (x ", hinge_x + 60, ") до отвору скидання: ", hinge_x + 60 - x_drop, " мм"));
assert(width <= 200, "ширше за 200 мм");

// =========================== модулі ===================================
module teardrop_x(d, h) {
    rotate([0, 90, 0]) linear_extrude(height = h, center = true)
        hull() { circle(d = d); translate([-d/2*sqrt(2), 0]) square(0.01, center = true); }
}

module tt_ghost() {
    translate([gb, motor_y, zs - tt_h/2]) cube([tt_bl, tt_t, tt_h]);
    translate([x_can, motor_y + tt_t/2, zs]) rotate([0, 90, 0]) cylinder(d = 15, h = gb - x_can);
}

module enc_ghost() {
    translate([x_fa, motor_y - iw - enc_slot_w/2, zs]) rotate([90, 0, 0])
        cylinder(d = enc_disc_d, h = 1.5, center = true);
}

module sleeve() {
    difference() {
        union() {
            translate([x_can - 1, motor_y - iw, z_fl_bot]) cube([gf + mc + 2.5 - (x_can - 1), tt_t + iw, 2.5]);
            translate([x_can - 1, motor_y - iw, z_fl_bot]) cube([10, tt_t + iw + 4, 2.5]);
            translate([gb - mc - 2, motor_y - iw, z_rf_bot]) cube([gf - gb + 2*mc + 4.5, tt_t + iw, 1.6]);
            translate([gb - mc - 2, motor_y - iw, z_fl_bot]) cube([gf - gb + 2*mc + 4.5, iw, z_rf_top - z_fl_bot]);
            translate([gf + mc, motor_y - iw, z_fl_bot]) cube([2.5, tt_t + iw, z_rf_top - z_fl_bot]);
            translate([gb - mc - 2, motor_y, z_fl_top - 0.01]) cube([2, tt_t, 1.0]);
            translate([gb - mc - 2, motor_y, z_rf_bot - 0.8]) cube([2, tt_t, 0.81]);
        }
        translate([x_fa, motor_y - iw - 1, zs]) rotate([-90, 0, 0]) cylinder(d = tt_boss_d, h = iw + 2);
        for (yy = [motor_y - 2.3, y_out + 0.5])
            translate([x_can + 2.5, yy, z_fl_bot - 1]) cube([4, 2, 5]);
    }
}

module rocker() {
    difference() {
        union() {
            sleeve();
            mirror([0, 1, 0]) sleeve();
            for (wx = [lx0, lx1]) {
                translate([wx, -motor_y, z_fl_bot]) cube([4, 2*motor_y, z_rf_top - z_fl_bot]);
                hull() {
                    translate([wx, -pin_r, z_rf_top - 1]) cube([4, 2*pin_r, 1]);
                    translate([wx, 0, z_p]) rotate([0, 90, 0]) cylinder(r = pin_r, h = 4);
                }
            }
            translate([lx0, -motor_y, z_rf_top - 4]) cube([lx1 + 4 - lx0, 2*motor_y, 4]);
        }
        for (wx = [lx0, lx1]) translate([wx + 2, 0, z_p]) teardrop_x(pivot_d + 0.4, 6);
        translate([lx0 + 10, -lite_hw, z_rf_top - 5]) cube([lx1 - lx0 - 16, 2*lite_hw, 6]);
        if (enc_slot) for (s = [-1, 1])
            translate([x_fa - enc_disc_d/2 - 2,
                       s > 0 ? motor_y - iw - enc_slot_w : -(motor_y - iw),
                       zs - enc_disc_d/2 - 2])
                cube([enc_disc_d + 4, enc_slot_w, enc_disc_d + 8]);
    }
}

// де решітку НЕ прорізати (там вушка, упори, кронштейни, отвір, кріплення)
function hw_at(x) = x < x_split ? hw_rear : hw_front;
function solid_zone(x0, x1, y0, y1) =
       (x1 > x_drop - 20 && x0 < x_drop + 20 && y1 > -20 && y0 < 20)          // навколо отвору скидання
    || (x1 > lx0 - 12 && x0 < lx1 + 12 && y1 > -12 && y0 < 12)                // вушка качалки
    || (x1 > 26 && x0 < 44 && (y1 > 30 || y0 < -30))                          // упори качалки
    || (x1 > x_ra - 20 && x0 < x_ra + 20 && (y1 > 44 || y0 < -44))            // кронштейни задніх коліс
    || (x1 > hinge_x - 16);                                                   // кріплення плеча датчиків

module deck() {
    my  = mark_side * mark_y;
    cys = [-9.4, 9.4, my - 7.4, my + 7.4];
    m_yl = min(cys) - 2 - 6 + 4;
    m_yh = max(cys) + 2 + 6 - 4;
    difference() {
        union() {
            // плита
            translate([deck_x0, -hw_rear, z_deck]) cube([x_split - deck_x0, 2*hw_rear, deck_t]);
            translate([x_split - 0.01, -hw_front, z_deck]) cube([deck_x1 - x_split, 2*hw_front, deck_t]);
            // бортики знизу вздовж задньої частини (жорсткість)
            for (s = [-1, 1]) translate([deck_x0, s > 0 ? hw_rear - 2 : -hw_rear, z_deck - 5])
                cube([x_split - deck_x0, 2, 5.01]);
            translate([deck_x0, -hw_rear, z_deck - 5]) cube([2, 2*hw_rear, 5.01]);
            // вушка осі качалки
            for (lx = [lx0 - 5, lx1 + 5]) hull() {
                translate([lx, -pin_r, z_deck - 0.01]) cube([4, 2*pin_r, 0.02]);
                translate([lx, 0, z_p]) rotate([0, 90, 0]) cylinder(r = pin_r, h = 4);
            }
            // упори качалки (±7°)
            for (s = [-1, 1])
                translate([30, s > 0 ? 34 : -42, z_p + 1.3]) cube([10, 8, z_deck - z_p - 1.3 + 0.01]);
            // кронштейни задніх коліс
            for (s = [-1, 1]) {
                y0 = s > 0 ? hw_rear - br_t : -hw_rear;
                hull() {
                    translate([x_ra, y0, zs]) rotate([-90, 0, 0]) cylinder(r = 9, h = br_t);
                    translate([x_ra - 15, y0, z_deck - 0.01]) cube([30, br_t, 0.02]);
                }
            }
        }
        // решітка: вікна під стяжки, економія пластику й часу друку
        for (x = [deck_x0 + border : pw_ : deck_x1 - border - win],
             y = [-hw_rear + border : pw_ : hw_rear - border - win])
            if (y >= -hw_at(x) + border && y + win <= hw_at(x) - border
                && y >= -hw_at(x + win) + border && y + win <= hw_at(x + win) - border
                && !solid_zone(x, x + win, y, y + win))
                translate([x, y, z_deck - 1]) cube([win, win, deck_t + 2]);
        // вісь качалки
        translate([(lx0 + lx1)/2 + 2, 0, z_p]) rotate([0, 90, 0])
            cylinder(d = pivot_d + 0.3, h = lx1 - lx0 + 20, center = true);
        // отвори напіввісей
        for (s = [-1, 1]) translate([x_ra, s > 0 ? hw_rear - br_t - 1 : -hw_rear - 1, zs])
            rotate([-90, 0, 0]) cylinder(d = stub_d + 0.4, h = br_t + 2);
        // отвір скидання (під отвором револьвера)
        translate([x_drop, 0, z_deck - 1]) cylinder(d = drop_d, h = deck_t + 2);
        // кріплення плеча датчиків
        for (x = [hinge_x - 7, hinge_x + 7], y = [m_yl, m_yh])
            translate([x, y, z_deck - 1]) cylinder(d = 3.4, h = deck_t + 2);
    }
}

module pivot_pin() {
    x0 = lx0 - 6;
    x1 = lx1 + 13;
    difference() {
        union() {
            translate([x0 - 2.5, 0, 0]) rotate([0, 90, 0]) cylinder(d = pivot_d + 4, h = 2.5);
            translate([x0 - 0.01, 0, 0]) rotate([0, 90, 0]) cylinder(d = pivot_d, h = x1 - x0);
        }
        translate([x1 - 2, 0, -10]) cylinder(d = fil_d, h = 20);
        translate([-200, -50, -pivot_d/2 + 0.5 - 50]) cube([400, 100, 50]);
    }
}

module rear_stub() {
    jl = 3 + br_t + 0.5;
    difference() {
        union() {
            cylinder(d = stub_d, h = jl);
            translate([0, 0, jl]) cylinder(d = stub_d + 4, h = 1.5);
            translate([0, 0, jl + 1.5]) intersection() {
                cylinder(d = 5.3, h = 9);
                translate([-5, -1.8, 0]) cube([10, 3.6, 9]);
            }
        }
        translate([0, 0, 1.5]) rotate([90, 0, 0]) cylinder(d = fil_d, h = 10, center = true);
    }
}

module motor_test() {
    intersection() {
        rocker();
        translate([gb - 4, motor_y - 4, 0]) cube([gf - gb + 9, tt_t + 6, 100]);
    }
}

// =========================== вивід ====================================
if (part == "all") {
    color("gray")   deck();
    color("orange") rocker();
    color("silver") translate([0, 0, z_p]) pivot_pin();
    for (s = [-1, 1]) {
        color("yellow") mirror([0, s > 0 ? 0 : 1, 0]) tt_ghost();
        if (enc_slot) color("black") mirror([0, s > 0 ? 0 : 1, 0]) enc_ghost();
        %translate([x_fa, s*(y_wf + wheel_w/2), zs]) rotate([90, 0, 0]) cylinder(d = wheel_d, h = wheel_w, center = true);
        %translate([x_ra, s*(y_wr + wheel_w/2), zs]) rotate([90, 0, 0]) cylinder(d = wheel_d, h = wheel_w, center = true);
        color("teal") translate([x_ra, s*(hw_rear - br_t - 3), zs]) rotate([s > 0 ? -90 : 90, 0, 0]) rear_stub();
    }
    // де стоять револьвер і котушка (лише для перегляду)
    %translate([x_drop, 40, z_deck + deck_t]) cylinder(r = 48.6, h = 2);
    %translate([-68 - 44, -32 - 22, z_deck + deck_t]) cube([88, 44, 2]);
    %translate([-150, -110, -1]) cube([320, 220, 1]);
}
else if (part == "deck")       translate([0, 0, z_deck + deck_t]) rotate([180, 0, 0]) deck();   // догори дном: верх деки на столі
else if (part == "rocker")     translate([0, 0, -z_fl_bot]) rocker();
else if (part == "pivot_pin")  translate([0, 0, pivot_d/2 - 0.5]) pivot_pin();
else if (part == "rear_stub")  rear_stub();
else if (part == "motor_test") translate([0, 0, -z_fl_bot]) motor_test();
