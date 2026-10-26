// sonar.ino — блок далекоміра HC-SR04. Інший датчик -> переписати лише sonarCmOnce().

void sonarSetup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
}

int sonarCmOnce() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 25000UL);  // ~4 м
  if (us == 0) return 400;                              // нічого не бачить
  return us / 58;
}

// медіана з 3 вимірів — менше хибних спрацювань
int sonarCm() {
  int a = sonarCmOnce(); delay(8);
  int b = sonarCmOnce(); delay(8);
  int c = sonarCmOnce();
  if (a > b) { int t = a; a = b; b = t; }
  if (b > c) { int t = b; b = c; c = t; }
  if (a > b) { int t = a; a = b; b = t; }
  return b;
}
