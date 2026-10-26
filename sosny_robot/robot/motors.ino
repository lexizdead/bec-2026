// motors.ino — блок мотора (L298N, канал B: OUT3/OUT4). Мотор один, кермує рама.

void motorsSetup() {
  pinMode(PIN_IN3, OUTPUT); pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);
  stopAll();
}

// spd: -255 (назад) .. 255 (вперед)
void drive(int spd) {
  spd = constrain(spd, -255, 255);
  if (MOTOR_INVERT) spd = -spd;
  digitalWrite(PIN_IN3, spd > 0);
  digitalWrite(PIN_IN4, spd < 0);
  analogWrite(PIN_ENB, abs(spd));
}

// стоп з гальмуванням: обидва входи HIGH -> мотор закорочений, зупиняється швидше
void stopAll() {
  digitalWrite(PIN_IN3, HIGH);
  digitalWrite(PIN_IN4, HIGH);
  analogWrite(PIN_ENB, 255);
}
