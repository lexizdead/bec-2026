// ======================================================================
//  ДОВГА ВІСЬ НА ДВА КОЛЕСА · sosny · Т-150
//
//  Як wheel_axle_round.scad (v5), але D-кінці з ОБОХ боків:
//     колесо | D-кінець 29 | шайба | вал Ø6 у тримачі | шайба | D-кінець 29 | колесо
//  Вал крутиться в отворі Ø6.5 тримача axle_holder.scad (звужений, 18.4 мм).
//  Шайби окремі (2 шт): вісь просовується крізь тримач, надягаються шайби,
//  потім колеса натискаються на D-кінці і тримають усе разом.
//
//  Друк ЛЕЖАЧИ (так вісь у рази міцніша на згин, ніж стоячи):
//  PLA 0.2, 4 стінки, 100 %, без підтримок, лиски D-кінців дивляться вбік.
//  Знизу зрізано 0.4 мм, щоб вісь не котилась по столу.
// ======================================================================

part = "axle";     // "axle" | "washer" | "all"

// --- D-кінець під маточину колеса (як у v5, що добре сів) ---
hub_d    = 5.4;    // круглий діаметр
hub_flat = 3.5;    // між лисками
corner_r = 0.5;
hub_len  = 29;     // довжина D-кінця з кожного боку

// --- вал між колесами ---
ax_d     = 6;      // отвір тримача Ø6.5
holder_w = 18.4;   // товщина тримача (block_y у axle_holder.scad)
washer_t = 1.5;  washer_d = 10;  washer_fit = 0.2;
play     = 0.6;    // осьовий люфт
shaft_len = holder_w + 2*washer_t + play;   // 22.0 — між маточинами коліс

flat_cut = 0.4;    // зріз знизу для друку лежачи
$fn = 96;

total = 2*hub_len + shaft_len;
echo(str("Довжина осі: ", total, " мм; між колесами: ", shaft_len, " мм"));

module d_profile() {
    offset(r = corner_r) offset(delta = -corner_r)
        intersection() { circle(d = hub_d); square([hub_d + 1, hub_flat], center = true); }
}

module d_end() {   // уздовж +x, з фаскою на кінці
    rotate([0, 90, 0]) {
        linear_extrude(hub_len - 0.6) d_profile();
        translate([0, 0, hub_len - 0.6]) linear_extrude(0.6, scale = 0.8) d_profile();
    }
}

module axle() {
    difference() {
        union() {
            translate([-shaft_len/2, 0, 0]) rotate([0, 90, 0]) cylinder(d = ax_d, h = shaft_len);
            translate([ shaft_len/2, 0, 0]) d_end();
            translate([-shaft_len/2, 0, 0]) mirror([1, 0, 0]) d_end();
        }
        translate([-total, -10, -ax_d/2 - 10 + flat_cut]) cube([2*total, 20, 10]);
    }
}

module washer() {
    difference() {
        cylinder(d = washer_d, h = washer_t);
        translate([0, 0, -1]) cylinder(d = ax_d + washer_fit, h = washer_t + 2);
    }
}

if (part == "axle")   translate([0, 0, ax_d/2 - flat_cut]) axle();
if (part == "washer") washer();
if (part == "all") {
    translate([0, 0, ax_d/2 - flat_cut]) axle();
    for (i = [0, 1]) translate([-8 + 16*i, 14, 0]) washer();
}
