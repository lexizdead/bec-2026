// Тест моторів: M1 вперед/назад, M2 вперед/назад, обидва вперед
// Піни як на схемі day2_wiring.svg (ENA D25, IN1 D26, IN2 D27, IN3 RX2, IN4 TX2, ENB D18)
const int ENA = 25, IN1 = 26, IN2 = 27, IN3 = 16, IN4 = 17, ENB = 18;

void motor(int en, int a, int b, int v) {   // v: -255..255
  digitalWrite(a, v > 0); digitalWrite(b, v < 0); analogWrite(en, abs(v));
}

void step(const char *name, int l, int r, int ms) {
  Serial.println(name);
  motor(ENA, IN1, IN2, l); motor(ENB, IN3, IN4, r);
  delay(ms);
}

void setup() {
  Serial.begin(115200);
  for (int p : {ENA, IN1, IN2, IN3, IN4, ENB}) pinMode(p, OUTPUT);
}

void loop() {
  int s = 150;
  step("M1 (ліве) вперед", s, 0, 1000);
  step("M1 (ліве) назад", -s, 0, 1000);
  step("M2 (праве) вперед", 0, s, 1000);
  step("M2 (праве) назад", 0, -s, 1000);
  step("обидва вперед", s, s, 1500);
  step("стоп", 0, 0, 2000);
}
