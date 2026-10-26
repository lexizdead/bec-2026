// sosny — шарнір ламаної рами (як Т-150К) на SG90
// Дві деталі: FORK (вилка, передня половина, тримає серво валом вниз)
//             TONGUE (язик, задня половина, кріпиться до качалки + вісь M3 знизу)
// Вісь повороту вертикальна, x = P. Кут повороту ±35° без зачіпань.
// Кріплення до піврам: фланці 3 мм з 4 отворами M3 + 2 прорізи під стяжки.
//
// part = "fork" | "tongue" | "assembly"
part = "assembly";
print_orient = false;   // true = покласти на бік (як друкувати)

$fn = 48;
W  = 24;     // ширина всіх профілів (y)
P  = 28;     // x осі повороту
// висоти (z)
bot_t   = 4;            // нижнє плече вилки 0..4
gap_bot = 1.0;          // зазор під язиком (заповнити шайбами M3)
tng_t   = 8;            // товщина язика
tng_z0  = bot_t + gap_bot;        // 5
tng_z1  = tng_z0 + tng_t;         // 13
arm_gap = 9.5;          // від верху язика до низу верхнього плеча
top_z0  = tng_z1 + arm_gap;       // 22.5
top_t   = 3;
top_z1  = top_z0 + top_t;         // 25.5

// SG90
sv_len = 23.0; sv_w = 12.4;       // отвір під корпус
sv_shaft_off = 5.35;              // вал зміщений від центру корпусу
sv_tab_pitch = 27.8; sv_tab_hole = 1.8;
sv_cx = P - sv_shaft_off;         // центр корпусу

// качалка (подвійна, вздовж x)
horn_len = 34; horn_w = 6.8; horn_hub = 8; horn_depth = 1.6;

module plan_arm(x1, ch) {          // план плеча з фасками на кінці
    polygon([[0,-W/2],[x1-ch,-W/2],[x1,-W/2+ch],[x1,W/2-ch],[x1-ch,W/2],[0,W/2]]);
}
module xz_tri(pts) {               // трикутник у площині x-z на всю ширину
    translate([0,W/2,0]) rotate([90,0,0]) linear_extrude(W) polygon(pts);
}
module flange_holes(xs, xslot) {
    for (x = xs, y = [-6, 6]) translate([x,y,-1]) cylinder(d=3.4, h=10);
    for (y = [-7, 7]) translate([xslot-1, y-2.25, -1]) cube([2, 4.5, 10]);
}

module fork() {
    difference() {
        union() {
            translate([-30,-W/2,0]) cube([34, W, 3]);                // фланець
            translate([0,-W/2,0])  cube([4, W, top_z1]);             // задня стінка
            linear_extrude(bot_t) plan_arm(P+13, 6);                 // нижнє плече
            translate([0,0,top_z0]) linear_extrude(top_t) plan_arm(P+13, 6); // верхнє
            xz_tri([[4,bot_t],[10,bot_t],[4,bot_t+6]]);              // косинки
            xz_tri([[4,top_z0],[10,top_z0],[4,top_z0-6]]);
            xz_tri([[0,3],[-6,3],[0,9]]);
        }
        translate([P,0,-1]) cylinder(d=2.8, h=bot_t+2);              // M3 саморіз-вісь
        translate([sv_cx-sv_len/2, -sv_w/2, top_z0-1]) cube([sv_len, sv_w, top_t+2]);
        for (s=[-1,1]) translate([sv_cx+s*sv_tab_pitch/2, 0, top_z0-1])
            cylinder(d=sv_tab_hole, h=top_t+2);
        flange_holes([-10,-24], -17);
    }
}

module tongue() {
    difference() {
        union() {
            translate([0,0,tng_z0]) linear_extrude(tng_t)
                polygon([[P-11,-6],[P-5,-W/2],[P+18,-W/2],[P+18,W/2],[P-5,W/2],[P-11,6]]);
            translate([P+18,-W/2,0]) cube([4, W, tng_z1]);           // стінка
            translate([P+18,-W/2,0]) cube([34, W, 3]);               // фланець
            xz_tri([[P+18,tng_z0],[P+16,tng_z0],[P+18,tng_z0-2]]);
            xz_tri([[P+22,3],[P+27,3],[P+22,8]]);
        }
        translate([P,0,tng_z0-1]) cylinder(d=3.4, h=1+5.5);          // гніздо осі M3
        translate([P-horn_len/2, -horn_w/2, tng_z1-horn_depth]) cube([horn_len, horn_w, 5]);
        translate([P,0,tng_z1-horn_depth]) cylinder(d=horn_hub, h=5);
        flange_holes([P+30, P+44], P+37);
    }
}

module place(o) { if (print_orient) translate([0,0,W/2]) rotate([90,0,0]) children(); else children(); }

if (part == "fork")     place() fork();
if (part == "tongue")   place() tongue();
if (part == "assembly") { color("orange") fork(); color("steelblue") tongue(); }
