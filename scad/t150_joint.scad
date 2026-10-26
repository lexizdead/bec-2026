// sosny — зчленування рами "як у Т-150К" між двома платформами (Brilliant_Jaban_3)
//
// Як працює (як на Т-150К):
//   * вертикальна вісь (M3x20) — рама "ламається" вліво/вправо, так робот кермує;
//   * поздовжня вісь (M3x16) — половина B може закручуватись ±12° відносно A,
//     щоб на нерівностях усі 4 колеса торкались землі;
//   * SG90 висить у проміжку між платформами і через тягу штовхає "плече"
//     (як гідроциліндр повороту на Т-150К). Вага рами і удари йдуть на болт M3,
//     а не на вал серво.
//
// Деталі (part = ...):
//   "A_servo" — кронштейн на великій платформі A: гніздо SG90 + нижня щока шарніра
//   "B_arm"   — плече: верхня щока + важіль під тягу + вухо поздовжньої осі
//               (STL уже перевернуте для друку — плоским верхом на стіл)
//   "roll"    — опора поздовжньої осі на малій платформі B
//   "link"    — тяга між качалкою SG90 і важелем
//   "assembly"— збірка з платформами; крути steer / roll для перевірки
//
// Кріплеж: M3x20 (вертикальна вісь), M3x16 (поздовжня), 2 шайби M3,
//          4x M3 з гайками або стяжки (до платформ), 2x M2x8 (тяга), 2 саморізи з комплекту SG90.
//
// Координати: (0,0,0) = вертикальна вісь на рівні верху платформ, +y — до малої платформи B.
// Платформи як у Brilliant_Jaban_3 ×10 (1 од. Tinkercad = 1 см): 55×83 і 55×47, проміжок 43 мм.

part  = "assembly";
steer = 0;      // -32..32  кут ламання рами (лише перегляд)
roll  = 0;      // -12..12  закручування половини B (лише перегляд)
show_platforms = true;
$fn = 48;

// ---------- платформи ----------
plat_w = 55;  plat_t = 10;
gap = 43;               // проміжок між платформами
off = 2;                // вісь зсунута на 2 мм до B, щоб вліз SG90
yA  = -(gap/2 + off);   // край A = -23.5
yB  =   gap/2 - off;    // край B = +19.5

// ---------- SG90 ----------
sv_hole  = [22.9, 12.4];   // отвір під корпус (корпус 22.5 x 11.8)
sv_shaft = 5.35;           // вал від центру корпусу
sv_tab_pitch = 27.8;  sv_tab_len = 32.2;
sv_tab_z  = 15.9;          // від низу корпусу до низу вушок
sv_case_h = 22.7;
horn_above_tab = 15.5;     // від низу вушок до верху качалки (ВИМІРЯТИ свою!)
sv_c = [-6, yA + 1.2 + sv_hole[1]/2];   // центр корпусу
S    = [sv_c[0] + sv_shaft, sv_c[1]];    // вал серво

// ---------- тяга ----------
rh    = 9.5;     // отвір качалки (2-й-3-й від центру)
horn0 = -15;     // кут качалки в нейтралі
R     = 14;      // плече важеля (R/rh ≈ 1.5 -> у 1.5 раза більше зусилля)
H0 = S + rh*[cos(horn0), sin(horn0)];
P0 = [R, 0];
L  = norm(P0 - H0);       // довжина тяги між отворами

// ---------- висоти ----------
t_pl   = 5;                        // плита кронштейна A
z_top  = t_pl + horn_above_tab;    // 20.5 — верх плеча = верх качалки
boss_d = 16;
zA_boss = 8.5;                     // нижня щока 0..8.5
zB0    = zA_boss + 0.5;            // верхня щока 9..20.5 (шайба між ними)

// ---------- поздовжня вісь ----------
zr     = 7.5;          // висота поздовжньої осі
ear_y0 = 11;  ear_y1 = yB - 0.5;   // вухо плеча 11..19 (шайба до блока)
rb_w = 18; rb_d = 8; rb_h = 14;    // блок опори
roll_max = 12;
lip_z = zr + (rb_h - zr)*cos(roll_max) + rb_w/2*sin(roll_max);   // упор ±12°
lip_y1 = yB + 6;

// ================= деталі =================
module teardrop_y(d, h) {   // горизонтальний отвір уздовж +y, друкується без підтримок
    rotate([-90,0,0]) linear_extrude(h) hull() {
        circle(d=d);
        translate([0, -d/2*1.41]) square(0.01, center=true);
    }
}

module zip_slot(x, y) { translate([x-1, y-2.25, -1]) cube([2, 4.5, 30]); }

module A_servo() {
    difference() {
        union() {
            translate([-26, yA-22.5, 0]) cube([40, 22.5 - yA - 6, t_pl]);   // плита: на A + у проміжку
            translate([-26, sv_c[1]+sv_hole[1]/2 + 0.2, 0])
                cube([34, -4 - (sv_c[1]+sv_hole[1]/2 + 0.2), zA_boss]);    // передня перемичка
            cylinder(d=boss_d, h=zA_boss);                                  // нижня щока
            translate([10.6, yA-22.5, 0]) cube([3.4, 22.5 - yA - 6, 10]);   // ребро праве
            translate([-26,  yA-22.5, 0]) cube([3.4, 22.5 - yA - 4, 10]);   // ребро ліве
        }
        translate([sv_c[0]-sv_hole[0]/2, sv_c[1]-sv_hole[1]/2, -1])
            cube([sv_hole[0], sv_hole[1], 30]);                             // гніздо SG90
        for (s=[-1,1]) translate([sv_c[0]+s*sv_tab_pitch/2, sv_c[1], -1])
            cylinder(d=1.8, h=30);                                          // саморізи SG90
        translate([0,0,-1]) cylinder(d=2.8, h=30);                          // вісь M3 (саморіз)
        for (x=[-16, 4]) translate([x, yA-12, -1]) cylinder(d=3.4, h=30);   // M3 до платформи
        for (x=[-19, 7]) zip_slot(x, yA-19);                                // стяжки
    }
}

module B_arm() {
    difference() {
        union() {
            translate([0,0,zB0]) cylinder(d=boss_d, h=z_top-zB0);           // верхня щока
            hull() {                                                         // важіль
                translate([0,-4,z_top-4]) cube([1,8,4]);
                translate([R,0,z_top-4]) cylinder(d=8, h=4);
            }
            translate([-7, 0, zB0]) cube([14, ear_y0+0.01, z_top-zB0]);     // язик
            translate([-11, ear_y0, 0]) cube([22, ear_y1-ear_y0, z_top]);   // вухо
            translate([-rb_w/2-0.5, ear_y1-0.01, lip_z])
                cube([rb_w+1, lip_y1-ear_y1, z_top-lip_z]);                  // упор закручування
        }
        translate([0,0,-1]) cylinder(d=3.3, h=40);                          // вертикальна вісь
        translate([R,0,z_top-6]) cylinder(d=1.8, h=10);                     // M2 саморіз тяги
        translate([0, ear_y0-1, zr]) rotate([-90,0,0])
            cylinder(d=2.8, h=ear_y1-ear_y0+2);                             // поздовжня вісь M3
    }
}

module roll_mount() {
    difference() {
        union() {
            translate([-14, yB, 0]) cube([28, 18, 4]);                      // плита на B
            translate([-rb_w/2, yB, 0]) cube([rb_w, rb_d, rb_h]);           // блок
        }
        translate([0, yB-1, zr]) teardrop_y(3.4, rb_d+2);                   // поздовжня вісь
        for (x=[-9, 9]) translate([x, yB+13, -1]) cylinder(d=3.4, h=10);    // M3 до платформи
        for (x=[-11, 11]) zip_slot(x, yB+8+2.5);
    }
}

module link() {
    difference() {
        hull() { cylinder(d=7, h=3); translate([L,0,0]) cylinder(d=7, h=3); }
        for (x=[0, L]) translate([x,0,-1]) cylinder(d=2.3, h=5);
    }
}

// ================= збірка =================
function angdiff(a, b) = ((a - b) % 360 + 540) % 360 - 180;
// кут качалки для кута рами phi (перетин двох кіл)
function horn_ang(phi) =
    let(P = R*[cos(phi), sin(phi)], d = norm(P - S),
        a1 = (rh*rh - L*L + d*d)/(2*d), h = sqrt(max(0, rh*rh - a1*a1)),
        u = (P - S)/d, n = [-u[1], u[0]], B0 = S + a1*u,
        q1 = B0 + h*n, q2 = B0 - h*n,
        g1 = atan2(q1[1]-S[1], q1[0]-S[0]), g2 = atan2(q2[1]-S[1], q2[0]-S[0]),
        guess = horn0 + 1.5*phi)
    abs(angdiff(g1, guess)) < abs(angdiff(g2, guess)) ? g1 : g2;

module sg90() {
    zb = t_pl - sv_tab_z;
    color("dodgerblue") {
        translate([sv_c[0]-11.25, sv_c[1]-5.9, zb]) cube([22.5, 11.8, sv_case_h]);
        translate([sv_c[0]-sv_tab_len/2, sv_c[1]-5.9, t_pl]) cube([sv_tab_len, 11.8, 2.5]);
        translate([S[0], S[1], zb+sv_case_h]) cylinder(d=11.8, h=4);
    }
}
module horn(a) {
    color("white") translate([S[0], S[1], 0]) rotate(a) {
        translate([0,0,z_top-4.7]) cylinder(d=7, h=4.7);
        translate([0,0,z_top-1.5]) hull() { cylinder(d=6, h=1.5); translate([rh+3,0,0]) cylinder(d=4, h=1.5); }
    }
}
module platformA() {
    color("gold", 0.6) {
        translate([-27.5, -106.5, -plat_t]) cube([55, 83, plat_t]);
        translate([-7.5, -163.5, -plat_t]) cube([10, 60, plat_t]);
    }
}
module platformB() {
    color("gold", 0.6) {
        translate([-27.5, 19.5, -plat_t]) cube([55, 47, plat_t]);
        translate([-7.5, 66.5, -2*plat_t]) cube([10, 40, plat_t]);
    }
}
module roll_about_axis(a) { translate([0,0,zr]) rotate([0,a,0]) translate([0,0,-zr]) children(); }

module assembly(phi=steer, rl=roll) {
    a = horn_ang(phi);
    Q = S + rh*[cos(a), sin(a)];
    P = R*[cos(phi), sin(phi)];
    // половина A
    if (show_platforms) platformA();
    color("orange") A_servo();
    sg90();
    horn(a);
    // тяга
    color("red") translate([Q[0], Q[1], z_top]) rotate(atan2(P[1]-Q[1], P[0]-Q[0])) link();
    // половина B
    rotate([0,0,phi]) {
        color("steelblue") B_arm();
        roll_about_axis(rl) {
            color("seagreen") roll_mount();
            if (show_platforms) platformB();
        }
    }
}

if (part == "A_servo")  A_servo();
if (part == "B_arm")    translate([0,0,z_top]) rotate([180,0,0]) B_arm();   // плоским верхом на стіл
if (part == "roll")     roll_mount();
if (part == "link")     link();
if (part == "assembly") assembly();

echo(str("Тяга: між отворами L = ", L, " мм"));
echo(str("Кут качалки при рамі +30: ", angdiff(horn_ang(30), horn0), "  при -30: ", angdiff(horn_ang(-30), horn0)));
