// ======================================================================
//  КОТУШКА КАБЕЛЮ З ФРИКЦІЙНИМ ГАЛЬМОМ + КІЛЬЦЕ-НАПРЯМНА
//  (перша версія, ущільнена: менші зазори, ширша накладка гальма)
//  Робот «Швидкісний кабель-укладач», BEST Engineering Competition
//
//  Як працює: котушка крутиться на осі між двома стійками. Зверху на
//  ободах обох фланців лежить притискна пластина (гальмо). Вона шарнірно
//  закріплена ззаду (вісь — філамент 1.75), тому ходить угору-вниз
//  і прикриває котушку зверху. Гумка тягне передній край пластини до
//  гачка на основі — пластина тертям тримає котушку, кабель не
//  розмотується сам. Більше витків гумки = сильніше гальмо.
//  Мало тертя: наклейте на накладку пластини шар ізоленти.
//
//  Деталі:
//   spool_a  — фланець + серцевина зі спицями
//   spool_b  — другий фланець, вставляється в серцевину (+ термоклей)
//   stand    — основа зі стійками, гачок для гумки
//   axle     — вісь 8 мм, фіксується шматком філаменту в поперечному отворі
//   brake    — притискна пластина (друкується догори дном, так уже повернута)
//   guide    — кільце-напрямна: ззаду по центру під декою, кабель іде
//              низько між задніми колесами і не намотується на них
//  Кінець кабелю протягніть через два отвори в серцевині й зав'яжіть.
//  Друк: шар 0.28, 2 периметри, 15 %, без підтримок.
//  Координати: X вздовж осі котушки (поперек робота), Y вперед, Z вгору.
//  На роботі: центр основи x −68, y −32 (chassis.scad), кабель назад.
// ======================================================================

part = "all";   // [all, spool_a, spool_b, stand, axle, brake, guide]

// ---------- Кабель ----------
cable_d   = 0.5;
cable_len = 8000;   // мм (потрібно >= 7000)

// ---------- Котушка ----------
core_d    = 36;
inner_w   = 14;     // ширина між фланцями
flange_d  = 60;
flange_t  = 2.4;
axle_d    = 8;
clr       = 0.2;    // зазор отворів під вісь (малий: котушка не бовтається)

// ---------- Стійка ----------
up_t      = 4;
base_t    = 3;
gap       = 0.4;    // між фланцем і стійкою (малий: котушка не їздить уздовж осі)
fil_d     = 1.9;    // отвори під філамент 1.75 (тугенько)
pad_l     = 20;     // довжина накладки гальма вздовж Y (більше = більше тертя)

// ---------- Кільце-напрямна ----------
frame_h   = 56;     // висота низу деки над трасою (z_deck у chassis.scad)
guide_h   = 12;     // висота отвору кільця над трасою
guide_d   = 2.5;    // отвір кільця (вузький — кабель не бовтається)
$fn = 72;

// ======================= похідні величини =============================
R       = flange_d / 2;
H       = R + 4 + base_t;
x_in    = inner_w/2 + flange_t + gap;
x_out   = x_in + up_t;
core_id = core_d - 2*2.4;
fill    = 0.75;
r_full  = sqrt(pow(core_d/2, 2) + cable_len*cable_d*cable_d/(fill*PI*inner_w));
piv_y   = -(R + 4);
piv_z   = H + R + 2;

assert(r_full < R - 3, "кабель не влазить: збільште flange_d або inner_w");
echo(str("Намотка ", cable_len/1000, " м: радіус ", round(r_full*10)/10, " мм із ", R));
echo(str("Габарит вузла: ", 2*x_out, " x ", 2*R + 28, " x ", H + R + 5, " мм"));

// =========================== модулі ===================================
module teardrop_x(d, h) {
    rotate([0, 90, 0]) linear_extrude(height = h, center = true)
        hull() { circle(d = d); translate([-d/2*sqrt(2), 0]) square(0.01, center = true); }
}

module spool_a() {
    difference() {
        union() {
            cylinder(d = flange_d, h = flange_t);
            difference() {
                cylinder(d = core_d, h = flange_t + inner_w);
                translate([0, 0, -1]) cylinder(d = core_id, h = flange_t + inner_w + 2);
            }
            cylinder(d = axle_d + 7, h = flange_t + inner_w);
            for (a = [0, 60, 120]) rotate([0, 0, a])
                translate([-core_d/2 + 1, -1, 0]) cube([core_d - 2, 2, flange_t + inner_w]);
        }
        translate([0, 0, -1]) cylinder(d = axle_d + clr, h = flange_t + inner_w + 2);
        for (z = [flange_t + 4, flange_t + inner_w - 4])
            rotate([0, 0, 30]) translate([core_d/2 - 5, 0, z]) rotate([0, 90, 0]) cylinder(d = 1.5, h = 10);
    }
}

module spool_b() {
    sp_od = core_id - 0.2;
    difference() {
        union() {
            cylinder(d = flange_d, h = flange_t);
            difference() {
                cylinder(d = sp_od, h = flange_t + 4);
                translate([0, 0, -1]) cylinder(d = sp_od - 3.2, h = flange_t + 6);
            }
        }
        translate([0, 0, -1]) cylinder(d = axle_d + clr, h = flange_t + 2);
        for (a = [0, 60, 120]) rotate([0, 0, a])
            translate([-sp_od/2 - 1, -1.3, flange_t]) cube([sp_od + 2, 2.6, 5]);
    }
}

module stand() {
    difference() {
        union() {
            translate([-22, -(R + 12), 0]) cube([44, 2*R + 28, base_t]);
            for (s = [-1, 1]) translate([s > 0 ? x_in : -x_out, 0, 0])
                rotate([90, 0, 90]) linear_extrude(height = up_t) hull() {
                    translate([0, H]) circle(r = axle_d/2 + 5);
                    translate([piv_y, piv_z]) circle(r = 4.5);
                    translate([piv_y - 6, 0]) square([-(piv_y - 6) + 14, base_t]);
                }
            // гачок для гумки гальма
            translate([0, R + 10, 0]) cylinder(d = 5, h = base_t + 8);
            translate([0, R + 10, base_t + 8]) cylinder(d = 8, h = 2);
        }
        translate([0, 0, H]) teardrop_x(axle_d + clr, 2*x_out + 2);
        translate([0, piv_y, piv_z]) rotate([0, 90, 0]) cylinder(d = fil_d, h = 2*x_out + 2, center = true);
        // отвори під стяжки / гвинти до деки
        for (x = [-17, 17], y = [-(R + 7), R + 3])
            translate([x, y, -1]) cylinder(d = 3.6, h = base_t + 2);
    }
}

module axle_pin() {
    difference() {
        union() {
            translate([-x_out - 2.5, 0, 0]) rotate([0, 90, 0]) cylinder(d = axle_d + 5, h = 2.5);
            translate([-x_out - 0.01, 0, 0]) rotate([0, 90, 0]) cylinder(d = axle_d, h = 2*x_out + 4);
        }
        translate([x_out + 2, 0, -10]) cylinder(d = fil_d, h = 20);
        translate([-50, -50, -axle_d/2 + 0.6 - 50]) cube([100, 100, 50]);
    }
}

// притискна пластина: шарнір ззаду, накладка лежить на ободах обох фланців
module brake() {
    w = 2*x_in - 0.6;      // щільно між стійками, не перекошується
    difference() {
        union() {
            translate([-w/2, piv_y - 4, H + R + 1]) cube([w, (R + 12) - (piv_y - 4), 4]);   // пластина
            translate([-w/2, piv_y - 4, H + R - 1]) cube([w, 8, 6]);                        // блок шарніра
            // накладка на ободи: увігнута по радіусу фланця — лягає дугою, більше тертя
            difference() {
                translate([-w/2, -pad_l/2, H + R - 4]) cube([w, pad_l, 5.01]);
                translate([0, 0, H]) rotate([0, 90, 0]) cylinder(r = R, h = w + 2, center = true);
            }
        }
        translate([0, piv_y, piv_z]) rotate([0, 90, 0]) cylinder(d = fil_d, h = w + 2, center = true);
        translate([-3, R + 9, H + R]) cube([6, 10, 10]);                                    // паз для гумки
    }
}

module guide() {
    difference() {
        union() {
            hull() {
                translate([0, 0, guide_h]) rotate([90, 0, 0]) cylinder(r = 7, h = 6, center = true);
                translate([-5, -3, frame_h - 1]) cube([10, 6, 1]);
            }
            translate([-12, -3, frame_h]) cube([24, 25, 3]);
        }
        translate([0, 0, guide_h]) rotate([90, 0, 0]) {
            cylinder(d = guide_d, h = 10, center = true);
            translate([0, 0, 1.5])  cylinder(d1 = guide_d, d2 = guide_d + 3, h = 1.6);
            translate([0, 0, -3.1]) cylinder(d1 = guide_d + 3, d2 = guide_d, h = 1.6);
        }
        for (x = [-7, 7]) translate([x, 16, frame_h - 1]) cylinder(d = 3.4, h = 5);
    }
}

// =========================== вивід ====================================
if (part == "all") {
    color("gray") stand();
    translate([-(inner_w/2 + flange_t), 0, H]) rotate([0, 90, 0]) {
        color("orange") spool_a();
        color("gold") translate([0, 0, 2*flange_t + inner_w]) mirror([0, 0, 1]) spool_b();
        color("blue", 0.6) translate([0, 0, flange_t]) difference() {
            cylinder(r = r_full, h = inner_w);
            translate([0, 0, -1]) cylinder(d = core_d, h = inner_w + 2);
        }
    }
    color("silver") translate([0, 0, H]) axle_pin();
    color("teal") brake();
}
else if (part == "spool_a") spool_a();
else if (part == "spool_b") spool_b();
else if (part == "stand")   stand();
else if (part == "axle")    translate([0, 0, axle_d/2 - 0.6]) axle_pin();
else if (part == "brake")   translate([0, 0, H + R + 5]) rotate([180, 0, 0]) brake();
else if (part == "guide")   translate([0, 0, 12]) rotate([0, 90, 0]) guide();
