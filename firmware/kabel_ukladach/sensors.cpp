#include "sensors.h"
#include "config.h"
#include "settings.h"

static int readAvg(uint8_t pin) {
  // 2 відліки ~0.25 мс: гасить поодинокі викиди від ШІМ моторів
  return (analogRead(pin) + analogRead(pin)) >> 1;
}

static int norm(int raw, int w, int b) {
  if (b == w) return 0;
  long n = (long)(raw - w) * 1000L / (b - w);   // працює і якщо чорне < білого
  return (int)constrain(n, 0L, 1000L);
}

void sensorsBegin() {
  pinMode(PIN_LINE, INPUT);
  pinMode(PIN_MARK, INPUT);
}

void sensorsRead(Reading &r) {
  r.lineRaw = readAvg(PIN_LINE);
  r.markRaw = readAvg(PIN_MARK);
  r.line = norm(r.lineRaw, S.lineWhite, S.lineBlack);
  r.mark = norm(r.markRaw, S.markWhite, S.markBlack);
}

// Біле = середнє перших відліків (старт над білим), чорне = найдальший від нього екстремум.
static bool pickWB(long w0, int mn, int mx, int16_t &w, int16_t &b) {
  int dMin = abs((int)(w0 - mn)), dMax = abs((int)(mx - w0));
  if (dMax >= dMin) { w = mn; b = mx; } else { w = mx; b = mn; }
  return abs(b - w) >= 150;   // замалий контраст → датчик зависоко або не туди
}

bool sensorsCalibrate(unsigned long ms) {
  long sl = 0, sm = 0;
  for (int i = 0; i < 32; i++) { sl += analogRead(PIN_LINE); sm += analogRead(PIN_MARK); delay(5); }
  long wl = sl / 32, wm = sm / 32;
  int lMin = 1023, lMax = 0, mMin = 1023, mMax = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    int l = readAvg(PIN_LINE), m = readAvg(PIN_MARK);
    lMin = min(lMin, l); lMax = max(lMax, l);
    mMin = min(mMin, m); mMax = max(mMax, m);
    digitalWrite(PIN_LED, (millis() / 100) & 1);   // швидке блимання
  }
  digitalWrite(PIN_LED, LOW);
  int16_t lw, lb, mw, mb;
  bool okL = pickWB(wl, lMin, lMax, lw, lb);
  bool okM = pickWB(wm, mMin, mMax, mw, mb);
  Serial.print(F("calib line ")); Serial.print(lw); Serial.print('/'); Serial.print(lb);
  Serial.println(okL ? F(" OK") : F(" МАЛИЙ КОНТРАСТ"));
  Serial.print(F("calib mark ")); Serial.print(mw); Serial.print('/'); Serial.print(mb);
  Serial.println(okM ? F(" OK") : F(" МАЛИЙ КОНТРАСТ"));
  if (okL) { S.lineWhite = lw; S.lineBlack = lb; }
  if (okM) { S.markWhite = mw; S.markBlack = mb; }
  return okL && okM;
}
