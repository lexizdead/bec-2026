// ======================================================================
//  ВІЛЬНЕ КОЛЕСО: ТРИМАЧ + ВІСЬ · sosny · BEST EC Day 2 (TD)
//
//  Для жовтих коліс Ø66 × 30 мм без мотора. Тримач має ТАКУ САМУ полицю
//  і прорізи під стяжки, як motor_holder.scad (крок ламелей 20 мм),
//  а отвір осі — на тій самій висоті, що вал TT-мотора, тож усі 4 колеса
//  стоять рівно. Деталі симетричні — підходять на будь-який бік.
//  Друкувати: 2 × holder, 2 × axle.
//
//  Вісь: головка всередині → крізь тримач → шайба → D-кінець у маточину
//  колеса. Колесо сидить на D-кінці, вісь крутиться в отворі тримача.
//
//  Друк: PLA 0.28, 3 стінки, 30 %, без підтримок.
//  Тримач — полицею на стіл. Вісь — лежачи на плоскій грані (так міцніша).
// ======================================================================

part = "both";   // "holder", "axle" або "both"

// --- маточина колеса (D-отвір як на валу TT) ---
hub_d    = 5.4;    // круглий діаметр D-кінця
hub_flat = 3.0;    // між лисками (заміряно 3 мм; у типового TT — 3.7)
hub_len  = 9;      // скільки заходить у маточину

// --- вісь ---
ax_d     = 6;      // діаметр частини, що крутиться в тримачі
head_d   = 10;     // головка (не дає осі вийти назовні)
head_t   = 2.5;
washer_d = 10;     // шайба між тримачем і колесом
washer_t = 1.5;
play     = 0.6;    // осьовий люфт

// --- тримач (полиця = як у motor_holder.scad) ---
len_x    = 32;
flange_t = 3;
flange_in = 26;    // як у motor_holder: полиця тільки всередину рами
slot_l   = 4.4;
slot_w   = 2.2;
slat_pitch = 20;
slat_w   = 5;
block_x  = 20;     // довжина блоку вздовж руху
block_y  = 25.4;   // = ширина тунелю motor_holder: зовнішня грань врівень з краєм рами
axle_z   = 14.7;   // від рами до центру осі (= центр вала TT у motor_holder)
hole_fit = 0.5;    // зазор отвору (вісь має крутитись вільно)
below    = 6;      // матеріал під отвором

$fn = 48;

module holder() {
    out_y = 25.4;  // як тунель motor_holder — прорізи на тих самих місцях
    difference() {
        union() {
            translate([0, -flange_in, 0]) cube([len_x, flange_in + block_y/2, flange_t]);
            translate([(len_x - block_x)/2, -block_y/2, 0])
                cube([block_x, block_y, axle_z + below]);
        }
        // отвір осі (Ø6.5 горизонтально друкується без підтримок)
        translate([len_x/2, 0, axle_z]) rotate([90, 0, 0])
            cylinder(d = ax_d + hole_fit, h = block_y + 2, center = true);
        xc0 = (len_x - slat_pitch)/2;
        for (xc = [xc0, xc0 + slat_pitch], dx = [-1, 1])
            translate([xc + dx*(slat_w/2 + 0.5 + slot_w/2) - slot_w/2,
                       -(out_y/2 + 7) - slot_l/2, -1])
                cube([slot_w, slot_l, flange_t + 2]);
    }
}

module axle() {
    l_round = block_y + play + washer_t;
    // лежить уздовж X, плоска грань (z = −hub_flat/2) на столі
    translate([0, 0, hub_flat/2])
    intersection() {
        union() {
            rotate([0, 90, 0]) cylinder(d = head_d, h = head_t);
            translate([head_t, 0, 0]) rotate([0, 90, 0]) cylinder(d = ax_d, h = block_y + play);
            translate([head_t + block_y + play, 0, 0]) rotate([0, 90, 0]) cylinder(d = washer_d, h = washer_t);
            translate([head_t + l_round, 0, 0]) rotate([0, 90, 0])
                intersection() {   // D-кінець з двома лисками
                    cylinder(d = hub_d, h = hub_len);
                    translate([-hub_flat/2, -hub_d, 0]) cube([hub_flat, 2*hub_d, hub_len]);
                }
        }
        // зрізати все нижче лиски — плоска опора для друку
        translate([-1, -50, -hub_flat/2]) cube([100, 100, 100]);
    }
}

if (part == "holder" || part == "both") holder();
if (part == "axle"   || part == "both") translate([0, 40, 0]) axle();
