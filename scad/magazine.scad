// ======================================================================
//  МАГАЗИН ШКАНТІВ 9 x (90 x 10 мм) З ПОВЗУНКОМ НА SG90
//  Робот «Швидкісний кабель-укладач», BEST Engineering Competition
//
//  Принцип: шканти лежать стосом поперек робота. Внизу повзунок з
//  кишенею рівно на один шкант. Сервопривід через тягу (скріпка або
//  дріт 1 мм із Z-загином) зсуває повзунок на S мм вперед: шкант
//  падає в отвір у дні, а суцільна частина повзунка тим часом тримає
//  решту стосу. Повзунок повертається, і наступний шкант падає в кишеню.
//
//  Деталі: base (дно, тунель, кронштейн SG90), tower (канал),
//  slider (повзунок), test (низ каналу, щоб перевірити шканти, ~25 хв).
//  Башта стає на основу, фіксується 4 штифтами з філаменту 1.75 + клей.
//  У рамі під отвором для скидання потрібен виріз ~95 x 14 мм.
//
//  ВАЖЛИВО: дерев'яні шканти бувають 9.7-10.4 мм. Поміряйте кілька
//  і впишіть у dowel_d НАЙБІЛЬШИЙ діаметр.
//  Друк: PLA/PETG, шар 0.2, 3 периметри, 20 %, без підтримок.
//  Координати: X поперек робота (вздовж шканта), Y вперед, Z вгору.
// ======================================================================

part = "all";   // [all, base, tower, slider, test]

// ---------- Шканти ----------
dowel_d  = 10;     // діаметр (найбільший з поміряних)
dowel_l  = 90;     // довжина
n_dowels = 9;
clr_d    = 0.8;    // зазор по діаметру в каналі
clr_l    = 1.5;    // зазор по довжині
wall     = 2.4;
floor_t  = 2.4;
fil_d    = 2.0;    // отвори під штифти з філаменту 1.75
$fn = 48;

// ---------- SG90 ----------
sv_l     = 23.2;   // виріз під корпус, довжина
sv_w     = 12.8;   // виріз під корпус, ширина
sv_hole  = 27.8;   // відстань між отворами кріплення
sv_shaft = 5.6;    // зміщення валу від центру корпусу
horn_off = 10.5;   // від фланця серво до площини качалки
link_len = 20;     // довжина тяги між отворами

// ======================= похідні величини =============================
cx   = dowel_l + clr_l;    // канал, X
cy   = dowel_d + clr_d;    // канал, Y
pw   = dowel_d + 1.0;      // кишеня повзунка, Y
px   = dowel_l + 2.0;      // кишеня повзунка, X
st   = dowel_d - 0.2;      // товщина повзунка
th   = st + 0.4;           // висота тунелю
S    = pw + 3;             // хід повзунка
sl_m = 4;                  // поля повзунка збоку від кишені
sw   = px + 2*sl_m;        // ширина повзунка
tw   = sw + 0.6;           // ширина тунелю
bw   = 4;                  // бічні стінки основи
lip  = 2.5;                // губи, що притримують повзунок
lip_t = 1.2;
sl_y0 = -(S + 3);          // задній край повзунка
sl_y1 = pw + 3;            // передній край повзунка
by0  = sl_y0;              // основа по Y
by1  = S + sl_y1;
z_lip = floor_t + th;      // верх тунелю = низ башти
tower_h = n_dowels*dowel_d + 8;
ox   = tw/2 + bw;          // зовнішня півширина основи

x_tab   = 30;              // де на повзунку вушко для тяги
tab_l   = 12;
plate_x = x_tab - horn_off - 3;
sv_zc   = 12;              // висота осі серво над дном
tab_hole_y = sl_y0 - 8;
shaft_y = tab_hole_y + S/2 - link_len;
sv_yc   = shaft_y - sv_shaft;
br_y0   = sv_yc - sv_hole/2 - 4;

echo(str("Хід повзунка S = ", S, " мм; потрібне плече качалки ~", round(S/1.414*10)/10, " мм при повороті ±45°"));
echo(str("Висота магазину ", z_lip + tower_h, " мм; ширина ", 2*ox, " мм"));
echo(str("Отвір у рамі під скидання: X ", px + 1, " x Y ", pw + 1, " мм, центр на Y = ", S + pw/2));

// =========================== модулі ===================================
module base() {
    difference() {
        union() {
            // дно
            translate([-ox, by0, 0]) cube([2*ox, by1 - by0, floor_t]);
            // бічні стінки тунелю
            for (s = [-1, 1]) translate([s > 0 ? tw/2 : -ox, by0, 0]) cube([bw, by1 - by0, z_lip]);
            // губи поза баштою
            for (s = [-1, 1], seg = [[by0, -wall - 0.3], [cy + wall + 0.3, by1]])
                translate([s > 0 ? tw/2 - lip : -ox, seg[0], z_lip])
                    cube([lip + bw, seg[1] - seg[0], lip_t]);
            // вушка кріплення до рами
            for (s = [-1, 1], y = [by0, by1 - 10])
                translate([s > 0 ? ox : -ox - 9, y, 0]) cube([9, 10, floor_t]);
            // кронштейн SG90: полиця + вертикальна пластина
            translate([plate_x - 18, br_y0, 0]) cube([21, by0 - br_y0 + 0.01, floor_t]);
            translate([plate_x, br_y0, 0]) cube([3, by0 - br_y0 + 0.01, sv_zc + sv_w/2 + 4]);
        }
        // отвір для скидання
        translate([-(px/2 + 0.5), S - 0.5, -1]) cube([px + 1, pw + 1, floor_t + 2]);
        // отвори вушок
        for (s = [-1, 1], y = [by0 + 5, by1 - 5])
            translate([s*(ox + 4.5), y, -1]) cylinder(d = 3.4, h = floor_t + 2);
        // гнізда штифтів башти
        for (s = [-1, 1], y = [1, cy - 1])
            translate([s*(tw/2 + bw/2), y, z_lip - 6]) cylinder(d = fil_d, h = 7);
        // виріз і отвори під SG90 (вал до магазину)
        translate([plate_x - 1, sv_yc - sv_l/2, sv_zc - sv_w/2]) cube([5, sv_l, sv_w]);
        for (dy = [-1, 1])
            translate([plate_x - 1, sv_yc + dy*sv_hole/2, sv_zc]) rotate([0, 90, 0]) cylinder(d = 2, h = 5);
    }
}

module tower() {
    difference() {
        union() {
            // ніжки на стінках основи (по всій ширині, низ = зріз над повзунком)
            translate([-ox, -wall, 0]) cube([2*ox, cy + 2*wall, 2]);
            // стінки каналу
            translate([-(cx/2 + wall), -wall, 0]) cube([cx + 2*wall, cy + 2*wall, tower_h]);
            // ребра жорсткості
            for (x = [-cx/4, cx/4], yy = [-wall - 1.6, cy + wall])
                translate([x - 1, yy, 2]) cube([2, 1.6, tower_h - 2]);
        }
        // канал
        translate([-cx/2, 0, -1]) cube([cx, cy, tower_h + 2]);
        // фаска знизу передньої стінки: товстіший шкант не заклинить
        translate([0, cy, 0]) rotate([45, 0, 0]) cube([cx, 1.4, 1.4], center = true);
        // віконце, щоб бачити залишок
        translate([-4, cy - 1, 10]) cube([8, wall + 3, tower_h - 18]);
        // штифти
        for (s = [-1, 1], y = [1, cy - 1])
            translate([s*(tw/2 + bw/2), y, -1]) cylinder(d = fil_d, h = 4);
    }
}

module slider() {
    ch = 0.8;
    difference() {
        union() {
            translate([-sw/2, sl_y0, 0]) cube([sw, sl_y1 - sl_y0, st]);
            // вушко для тяги
            translate([x_tab - 1.5, sl_y0 - tab_l, 0]) cube([3, tab_l + 0.01, st]);
        }
        // кишеня на один шкант
        translate([-px/2, 0, -1]) cube([px, pw, st + 2]);
        // фаски по верху кишені
        for (y = [0, pw]) translate([0, y, st]) rotate([45, 0, 0]) cube([px, ch*1.414, ch*1.414], center = true);
        // два отвори для тяги (вибрати хід)
        for (y = [tab_hole_y, tab_hole_y + 4])
            translate([x_tab - 3, y, st/2]) rotate([0, 90, 0]) cylinder(d = 1.5, h = 6);
    }
}

module test() {
    intersection() {
        tower();
        translate([-100, -50, -1]) cube([200, 100, 16]);
    }
}

// =========================== вивід ====================================
if (part == "all") {
    color("gray")   base();
    color("orange") translate([0, 0, z_lip]) tower();
    color("teal")   translate([0, 0, floor_t]) slider();
    for (i = [0 : n_dowels - 1])
        color("burlywood") translate([0, cy/2, floor_t + dowel_d/2 + i*dowel_d])
            rotate([0, 90, 0]) cylinder(d = dowel_d, h = dowel_l, center = true);
}
else if (part == "base")   base();
else if (part == "tower")  tower();
else if (part == "slider") slider();
else if (part == "test")   test();
