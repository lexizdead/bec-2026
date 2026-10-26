// avoid.ino — блок об'їзду перешкоди ламаною рамою.
// стоп -> трохи назад -> рама вбік, поки не вільно -> рівно -> рама в інший бік (на курс)

int lastSide = 1;   // чергуємо боки: +1 вправо, -1 вліво

// Їдемо зламаною рамою в бік side, поки не стане вільно.
// Повертає час бокової фази, або 0 якщо не вийшло.
unsigned long goSide(int side) {
  setSteer(side * STEER_MAX);
  delay(STEER_SETTLE);
  drive(SPEED_TURN);
  unsigned long t0 = millis();
  while (millis() - t0 < SIDE_MAX_MS) {
    int d = sonarCm();
    if (d < STOP_CM) break;
    if (d > CLEAR_CM) {
      delay(EXTRA_MS);
      return millis() - t0;
    }
  }
  stopAll();
  return 0;
}

void backUp(unsigned long ms) {
  setSteer(0);
  delay(STEER_SETTLE);
  drive(-SPEED_SLOW);
  delay(ms);
  stopAll();
}

void avoid() {
  stopAll();
  backUp(BACK_MS);

  int side = -lastSide;
  unsigned long tSide = goSide(side);
  if (tSide == 0) {                       // не вийшло — в інший бік
    backUp(BACK_MS * 2);
    side = -side;
    tSide = goSide(side);
    if (tSide == 0) { setSteer(0); return; }
  }
  lastSide = side;

  setSteer(0); delay(STEER_SETTLE);       // повз перешкоду
  drive(SPEED_FWD);
  delay(STRAIGHT_MS);

  setSteer(-side * STEER_MAX);            // назад на курс
  delay(STEER_SETTLE);
  drive(SPEED_TURN);
  delay(tSide);
  setSteer(0);
}
