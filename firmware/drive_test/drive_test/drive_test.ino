// Тест моторів + HC-SR04. Піни як на схемі day2_wiring.svg.
// У моніторі: g = їхати, x = стоп, m = тест моторів по черзі
const int ENA = 25, IN1 = 26, IN2 = 27, IN3 = 16, IN4 = 17, ENB = 18;
const int TRIG = 15, ECHO = 39;   // D15, VN
const int SPEED = 130, SLOW_CM = 35, STOP_CM = 15;
bool go = false;

void motor(int en, int a, int b, int v) { digitalWrite(a, v > 0); digitalWrite(b, v < 0); analogWrite(en, abs(v)); }
void drive(int l, int r) { motor(ENA, IN1, IN2, l); motor(ENB, IN3, IN4, r); }
void brake() {
  for (int p : {IN1, IN2, IN3, IN4}) digitalWrite(p, HIGH);
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}
float cm() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10); digitalWrite(TRIG, LOW);
  unsigned long t = pulseIn(ECHO, HIGH, 25000);
  return t ? t / 58.0 : 999;
}
void motorSequence() {
  const char *n[] = {"M1 вперед", "M1 назад", "M2 вперед", "M2 назад"};
  int l[] = {SPEED, -SPEED, 0, 0}, r[] = {0, 0, SPEED, -SPEED};
  for (int i = 0; i < 4; i++) { Serial.println(n[i]); drive(l[i], r[i]); delay(1000); }
  drive(0, 0); Serial.println("тест моторів OK");
}

void setup() {
  Serial.begin(115200);
  for (int p : {ENA, IN1, IN2, IN3, IN4, ENB, TRIG}) pinMode(p, OUTPUT);
  pinMode(ECHO, INPUT);
  drive(0, 0);
  Serial.println("Мотори вимкнені. g = їхати, x = стоп, m = мотори по черзі");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'g') { go = true;  Serial.println(">>> ЇДУ"); }
    if (c == 'x') { go = false; Serial.println(">>> СТОП"); }
    if (c == 'm') { go = false; motorSequence(); }
  }
  float d = cm();
  const char *st;
  if (!go)               { drive(0, 0); st = "мотори вимк."; }
  else if (d < STOP_CM)  { brake();     st = "СТОП (перешкода)"; }
  else if (d < SLOW_CM)  { drive(SPEED * 0.4, SPEED * 0.4); st = "повільно"; }
  else                   { drive(SPEED, SPEED); st = "їду"; }

  static unsigned long t = 0;
  if (millis() - t > 200) { t = millis(); Serial.printf("%6.1f см  %s\n", d, st); }
  delay(60);
}
