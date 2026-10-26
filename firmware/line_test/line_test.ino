// ======================================================================
//  line_test.ino — ТІЛЬКИ рух по чорній лінії (перевірка до здачі)
//  Робот «Кабель-укладач», схема E3: передній привід, 2 мотори TT,
//  B1 = TCRT5000 лінії на A0 (над ЛІВИМ краєм смужки), B2 = мітки на A3.
//  Піни: ENA D5, IN1 D7, IN2 D8, ENB D6, IN3 D11, IN4 D12, кнопка D2.
//
//  Як користуватись:
//   1. Залити, відкрити Serial Monitor 115200. Поки робот стоїть, він
//      друкує сирі значення датчиків — так перевіряється розміщення.
//   2. Поставити робота на лінію: датчик B1 над лівим краєм смужки.
//   3. Натиснути «Старт»: робот покрутиться вліво-вправо (калібрування),
//      потім поїде. Повторне натискання — стоп.
// ======================================================================

#define ENA 5
#define IN1 7
#define IN2 8
#define ENB 6
#define IN3 11
#define IN4 12
#define BTN 2
#define LINE A0
#define MARK A3

// ---- налаштування (підбирати на трасі) ----
const bool LEFT_EDGE = true;   // false, якщо B1 стоїть над ПРАВИМ краєм
const bool INV_L = false;      // true, якщо лівий мотор крутиться назад
const bool INV_R = false;      // true, якщо правий мотор крутиться назад
int   BASE = 120;              // швидкість (0–255); почніть з 100–120
float KP   = 0.35;             // більше — різкіше кермо
float KD   = 3.0;              // гасить «виляння»
const int CAL_PWM = 110;       // швидкість повороту при калібруванні

int white = 100, black = 900;  // перезапишуться калібруванням
float prevErr = 0;
bool running = false;
unsigned long lostSince = 0, tCtl = 0;

void motor(int en, int a, int b, int v, bool inv) {
  if (inv) v = -v;
  v = constrain(v, -255, 255);
  digitalWrite(a, v >= 0 ? HIGH : LOW);
  digitalWrite(b, v >= 0 ? LOW : HIGH);
  analogWrite(en, abs(v));
}
void drive(int l, int r) { motor(ENA, IN1, IN2, l, INV_L); motor(ENB, IN3, IN4, r, INV_R); }
void stopM() { analogWrite(ENA, 0); analogWrite(ENB, 0); }

int rawLine() { return (analogRead(LINE) + analogRead(LINE)) / 2; }

// 0 = біле, 1000 = чорне
int readLine() {
  long n = (long)(rawLine() - white) * 1000L / (black - white);
  return (int)constrain(n, 0L, 1000L);
}

bool pressed() {
  if (digitalRead(BTN) == LOW) {
    delay(30);
    if (digitalRead(BTN) == LOW) { while (digitalRead(BTN) == LOW) delay(5); return true; }
  }
  return false;
}

// Робот крутиться на місці вліво-вправо-вліво, датчик проходить через лінію.
bool calibrate() {
  int mn = 1023, mx = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < 2400) {
    unsigned long t = millis() - t0;
    int s = (t < 600 || t >= 1800) ? 1 : -1;
    drive(-s * CAL_PWM, s * CAL_PWM);
    int v = analogRead(LINE);
    mn = min(mn, v); mx = max(mx, v);
  }
  stopM();
  // У модулів TCRT5000 вихід AO зазвичай БІЛЬШИЙ над чорним.
  white = mn; black = mx;
  Serial.print(F("calib white=")); Serial.print(white);
  Serial.print(F(" black=")); Serial.println(black);
  return (black - white) >= 150;
}

void setup() {
  Serial.begin(115200);
  int pins[] = {ENA, IN1, IN2, ENB, IN3, IN4};
  for (int p : pins) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
  pinMode(BTN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println(F("line_test: line raw | mark raw  (натисніть Старт)"));
}

void loop() {
  if (!running) {
    static unsigned long tp = 0;
    if (millis() - tp > 200) {   // перевірка розміщення датчиків
      tp = millis();
      Serial.print(rawLine()); Serial.print(F(" | ")); Serial.println(analogRead(MARK));
    }
    if (pressed()) {
      delay(500);
      if (!calibrate()) {        // малий контраст: датчик зависоко або не над лінією
        Serial.println(F("МАЛИЙ КОНТРАСТ — перевірте висоту й положення B1"));
        for (int i = 0; i < 20; i++) { digitalWrite(LED_BUILTIN, i & 1); delay(80); }
        return;
      }
      delay(300);
      prevErr = 0; lostSince = 0; running = true;
      Serial.println(F("GO"));
    }
    return;
  }

  if (pressed()) { stopM(); running = false; Serial.println(F("STOP")); return; }

  if (micros() - tCtl < 4000) return;   // цикл 250 Гц
  tCtl = micros();

  int line = readLine();
  float err = line - 500;               // тримаємо 50 % чорного (край смужки)
  float corr = KP * err + KD * (err - prevErr);
  prevErr = err;
  if (!LEFT_EDGE) corr = -corr;
  // датчик над лівим краєм: більше чорного = з'їхали вправо → кермуємо вліво
  drive(BASE - (int)corr, BASE + (int)corr);

  // лінії немає (суцільне біле) довше 1.5 с → стоп
  if (line < 50) {
    if (!lostSince) lostSince = millis();
    else if (millis() - lostSince > 1500) { stopM(); running = false; Serial.println(F("LINE LOST")); }
  } else lostSince = 0;
}
