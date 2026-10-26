#include "settings.h"
#include "config.h"
#include <EEPROM.h>

#define SETTINGS_MAGIC (0xBE00 + SETTINGS_VERSION)

Settings S;

void settingsDefaults() {
  S.magic = SETTINGS_MAGIC;
  S.lineWhite = DEF_LINE_WHITE; S.lineBlack = DEF_LINE_BLACK;
  S.markWhite = DEF_MARK_WHITE; S.markBlack = DEF_MARK_BLACK;
  S.kp = DEF_KP; S.ki = DEF_KI; S.kd = DEF_KD;
  S.basePwm = DEF_BASE_PWM; S.slowPwm = DEF_SLOW_PWM; S.approachPwm = DEF_APPROACH_PWM;
  S.dropOffsetMm = DEF_DROP_OFFSET_MM;
  S.dropDelayMs = DEF_DROP_DELAY_MS;
}

void settingsLoad() {
  EEPROM.get(0, S);
  if (S.magic != SETTINGS_MAGIC) settingsDefaults();
}

void settingsSave() {
  S.magic = SETTINGS_MAGIC;
  EEPROM.put(0, S);
}

void settingsPrint() {
  Serial.print(F("line W/B ")); Serial.print(S.lineWhite); Serial.print('/'); Serial.println(S.lineBlack);
  Serial.print(F("mark W/B ")); Serial.print(S.markWhite); Serial.print('/'); Serial.println(S.markBlack);
  Serial.print(F("Kp ")); Serial.print(S.kp, 3);
  Serial.print(F("  Ki ")); Serial.print(S.ki, 4);
  Serial.print(F("  Kd ")); Serial.println(S.kd, 3);
  Serial.print(F("PWM base/slow/approach ")); Serial.print(S.basePwm); Serial.print('/');
  Serial.print(S.slowPwm); Serial.print('/'); Serial.println(S.approachPwm);
  Serial.print(F("drop offset ")); Serial.print(S.dropOffsetMm); Serial.print(F(" mm, delay "));
  Serial.print(S.dropDelayMs); Serial.println(F(" ms"));
  Serial.print(F("encoders ")); Serial.print(ENC_COUNT);
  Serial.print(F(", mm/tick ")); Serial.println(MM_PER_TICK, 2);
}
