// ======================================================================
//  ПЛЕЧЕ ДАТЧИКА ЛІНІЇ (1 x TCRT5000, їде по краю смужки) + ПЛЕЧЕ ДАТЧИКА МІТОК
//  + КРІПЛЕННЯ-ШАРНІР ДО РАМИ
//  Робот «Швидкісний кабель-укладач», BEST Engineering Competition
//
//  Як користуватися: виберіть part, F6 (Render), File > Export > STL.
//  part = "all" — лише перегляд складання, не для друку.
//  Деталі вже повернуті для друку: без підтримок, PLA/PETG,
//  шар 0.2, 3 периметри, заповнення 20 %.
//
//  Вісь шарніра: шматок філаменту 1.75 мм, кінці розплющити паяльником.
//  Плату TCRT5000 вставити в гніздо знизу планки (датчиком донизу),
//  закріпити краплею термоклею або гвинтом М3 у паз.
//  Система координат: X вперед, Y вліво/вправо, Z вгору, вісь шарніра в 0.
// ======================================================================

part = "all";   // [all, line_arm, mark_arm, mount, fit_test]

// ---------- Траса ----------
line_w     = 10;    // ширина смужки, мм

// ---------- Модуль TCRT5000 (ВИМІРЯЙТЕ свій перед друком!) ----------
board_w    = 10.0;  // ширина плати
board_l    = 42.5;  // довжина плати
board_t    = 1.6;   // товщина плати
sensor_h   = 8.0;   // висота датчика над платою (від плати до торця)
sensor_x   = 3.5;   // від переднього краю плати до центру датчика
hole_x     = 14.0;  // від переднього краю плати до отвору М3 (0 = без пазу)

// ---------- Геометрія ----------
sensor_gap = 4.0;   // торець датчика над трасою, 3-5 мм
line_y     = line_w/2; // датчик лінії над КРАЄМ смужки (PID тримає 50 % чорного)
arm_len    = 60;    // від осі шарніра до лінії датчиків
mark_y     = 40;    // зміщення датчика міток від центру смужки вбік (мітки ±200 мм)
mark_side  = 1;     // 1 = з одного боку, -1 = з іншого
frame_h    = 56;    // висота низу деки над трасою (z_deck у chassis.scad)

// ---------- Шарнір ----------
pin_d      = 1.75;  // вісь (філамент 1.75; для гвинта М3 поставте 3)
clr        = 0.3;   // зазор отворів
$fn = 48;

// ======================= похідні величини =============================
Rb       = (pin_d + 4.5) / 2;          // радіус втулки
plank_t  = 3;                          // товщина планки
z_top    = Rb;                         // верх планки = верх втулки
z_pl_b   = z_top - plank_t;            // низ планки (сюди прилягає плата)
z_tip    = z_pl_b - board_t - sensor_h;// торець датчика
z_skid   = z_tip - sensor_gap;         // низ полозків = поверхня траси
pin_h    = -z_skid;                    // висота осі шарніра над трасою
pocket_w = board_w + 0.4;
wall_t   = 1.6;
x_f      = arm_len + sensor_x;         // передній край плати
x_r      = x_f - board_l;              // задній край плати
skid_t   = 4;
beam_w   = 10;
main_barrel = 14;
mark_barrel = 10;
cheek_t  = 4;
mount_H  = frame_h - pin_h;            // від осі до низу рами

main_y1  = line_y + pocket_w/2 + wall_t + skid_t;
mark_y0  = mark_y - pocket_w/2 - wall_t;

assert(x_r > Rb + 4, "arm_len замале для такої довжини плати");
assert(mount_H > Rb + 4, "frame_h замале: рама занизько для цього плеча");
assert(mark_y0 - main_y1 >= 1.5, "mark_y замале: плече міток зачепить плече лінії");

echo(str("Вісь шарніра на висоті ", pin_h, " мм над трасою"));
echo(str("Вісь на ", mount_H, " мм нижче низу рами"));
echo(str("Датчик лінії: ", line_y, " мм від центру (край смужки ±", line_w/2, " мм)"));
echo(str("Датчик міток: ", mark_side*mark_y, " мм від центру смужки"));

// =========================== модулі ===================================
module skid(y) {
    rf = (z_top - z_skid) / 2 - 0.5;   // передній радіус (заїзд на пороги)
    hull() {
        translate([x_r - 1, y - skid_t/2, z_pl_b]) cube([board_l + 3.2, skid_t, plank_t]);
        translate([x_r + 3, y, z_skid + 4])
            rotate([90, 0, 0]) cylinder(r = 4, h = skid_t, center = true);
        translate([x_f + 2.2 - rf, y, z_skid + rf])
            rotate([90, 0, 0]) cylinder(r = rf, h = skid_t, center = true);
    }
}

// ys — Y-координати датчиків; skid_l/skid_r — полозки зліва/справа
module arm(ys, skid_l, skid_r, beam_y, barrel_l) {
    y0  = min(ys) - pocket_w/2 - wall_t - (skid_l ? skid_t : 0);
    y1  = max(ys) + pocket_w/2 + wall_t + (skid_r ? skid_t : 0);
    px0 = x_r - 1;
    px1 = x_f + 2.2;
    difference() {
        union() {
            // планка
            translate([px0, y0, z_pl_b]) cube([px1 - px0, y1 - y0, plank_t]);
            // бортики гнізд і передній упор
            for (y = ys) {
                for (s = [-1, 1])
                    translate([px0, y + s*(pocket_w/2 + wall_t/2) - wall_t/2, z_pl_b - board_t - 0.2])
                        cube([px1 - px0, wall_t, board_t + 0.21]);
                translate([x_f + 0.2, y - pocket_w/2, z_pl_b - board_t - 0.2])
                    cube([2, pocket_w, board_t + 0.21]);
            }
            // полозки
            if (skid_l) skid(y0 + skid_t/2);
            if (skid_r) skid(y1 - skid_t/2);
            // балка до шарніра
            translate([0, beam_y - beam_w/2, z_top - 6]) cube([px0 + 1, beam_w, 6]);
            // втулка шарніра
            translate([0, beam_y, 0]) rotate([90, 0, 0])
                cylinder(r = Rb, h = barrel_l, center = true);
        }
        // отвір осі
        translate([0, beam_y, 0]) rotate([90, 0, 0])
            cylinder(d = pin_d + clr, h = barrel_l + 2, center = true);
        // пази під гвинт М3 (±2 мм, бо отвір на модулях буває в різних місцях)
        if (hole_x > 0) for (y = ys)
            hull() for (dx = [-2, 2])
                translate([x_f - hole_x + dx, y, z_pl_b - 1]) cylinder(d = 3.4, h = plank_t + 2);
    }
}

module line_arm() { arm([line_y], true, true, 0, main_barrel); }
module mark_arm() { arm([mark_side*mark_y], mark_side < 0, mark_side > 0, mark_side*mark_y, mark_barrel); }

// кріплення: пластина під низ рами + щоки з отворами під вісь
module mount() {
    ys_main = main_barrel/2 + 0.4 + cheek_t/2;
    ys_mark = mark_barrel/2 + 0.4 + cheek_t/2;
    my  = mark_side * mark_y;
    cys = [-ys_main, ys_main, my - ys_mark, my + ys_mark];
    yl  = min(cys) - cheek_t/2 - 6;
    yh  = max(cys) + cheek_t/2 + 6;
    re  = Rb + 1.5;
    difference() {
        union() {
            for (y = cys) hull() {
                translate([0, y, 0]) rotate([90, 0, 0]) cylinder(r = re, h = cheek_t, center = true);
                translate([-re, y - cheek_t/2, mount_H - 0.01]) cube([2*re, cheek_t, 0.01]);
            }
            translate([-12, yl, mount_H]) cube([24, yh - yl, 3]);
        }
        for (y = cys) translate([0, y, 0]) rotate([90, 0, 0])
            cylinder(d = pin_d + clr, h = cheek_t + 2, center = true);
        for (y = [yl + 4, yh - 4], x = [-7, 7])
            translate([x, y, mount_H - 1]) cylinder(d = 3.4, h = 5);
    }
}

// короткий зріз планки: перевірити, чи плата сідає в гніздо (друк ~10 хв)
module fit_test() {
    intersection() {
        arm([0], true, true, 0, main_barrel);
        translate([x_f - 10, -50, -50]) cube([13, 100, 100]);
    }
}

// =========================== вивід ====================================
if (part == "all") {
    color("orange") line_arm();
    color("gold")   mark_arm();
    color("gray")   mount();
    %translate([-20, -70, z_skid - 0.5]) cube([arm_len + 40, 140, 0.5]);
    color("black") translate([-20, -line_w/2, z_skid - 0.4]) cube([arm_len + 40, line_w, 0.45]);
}
else if (part == "line_arm") translate([0, 0, z_top]) rotate([180, 0, 0]) line_arm();
else if (part == "mark_arm") translate([0, 0, z_top]) rotate([180, 0, 0]) mark_arm();
else if (part == "mount")    translate([0, 0, mount_H + 3]) rotate([180, 0, 0]) mount();
else if (part == "fit_test") translate([0, 0, z_top]) rotate([180, 0, 0]) fit_test();
