// ======================================================================
//  ДРУКОВАНЕ ЗАДНЄ ВІЛЬНЕ КОЛЕСО Ø66 · sosny · BEST EC Day 2 (TD)
//
//  Замість жовтого TT-колеса (на складі лише 3 шт → купуємо 2 під мотори).
//  Сідає на ту саму вісь із free_wheel.scad (D-кінець 5.4 / лиски 3.0).
//  Гладкий обід без гуми — у танкових поворотах ковзає боком, як і треба.
//  ЛЕГКА ВЕРСІЯ (12:10): обід 10 мм, товщина 3, диск 2.4 — пластику вдвічі
//  менше (10 см³ замість 20), колесо лише котиться й ковзає, навантаження мале.
//  Друкувати 2 шт.
//
//  Друк: PLA 0.28, 2 стінки, 15 %, диском на стіл, без підтримок.
// ======================================================================

wheel_d  = 66;     // = wheel_d у sensor_bracket.scad (усі колеса однакові)
width    = 10;     // ширина обода (було 18)
rim_t    = 3;      // товщина обода (було 4)
web_t    = 2.4;    // товщина диска (було 4)
hub_od   = 14;
hub_len  = 9;      // = hub_len осі у free_wheel.scad
hub_d    = 5.4;    // D-кінець осі
hub_flat = 3.0;    // між лисками (заміряно)
fit      = 0.25;   // зазор (тугіше → 0.15)
n_holes  = 6;

$fn = 96;

difference() {
    union() {
        difference() {                                     // обід
            cylinder(d = wheel_d, h = width);
            translate([0, 0, -1]) cylinder(d = wheel_d - 2*rim_t, h = width + 2);
        }
        cylinder(d = wheel_d - 2, h = web_t);              // диск
        cylinder(d = hub_od, h = max(hub_len, web_t));     // маточина
    }
    // D-отвір під вісь
    translate([0, 0, -1]) intersection() {
        cylinder(d = hub_d + fit, h = hub_len + 2);
        translate([-(hub_flat + fit)/2, -hub_d, 0]) cube([hub_flat + fit, 2*hub_d, hub_len + 2]);
    }
    // полегшення
    for (i = [0 : n_holes - 1]) rotate(i * 360 / n_holes)
        translate([(hub_od/2 + (wheel_d/2 - rim_t))/2, 0, -1])
            cylinder(d = (wheel_d/2 - rim_t - hub_od/2) - 6, h = web_t + 2);
}
