#include "motors.h"
#include "config.h"

static void setOne(uint8_t en, uint8_t a, uint8_t b, int v, bool inv) {
  if (inv) v = -v;
  v = constrain(v, -PWM_MAX, PWM_MAX);
  if (v >= 0) { digitalWrite(a, HIGH); digitalWrite(b, LOW); }
  else        { digitalWrite(a, LOW);  digitalWrite(b, HIGH); v = -v; }
  analogWrite(en, v);
}

void motorsBegin() {
  uint8_t pins[] = {PIN_ENA, PIN_IN1, PIN_IN2, PIN_ENB, PIN_IN3, PIN_IN4};
  for (uint8_t p : pins) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
}

void motorsSet(int left, int right) {
  setOne(PIN_ENA, PIN_IN1, PIN_IN2, left,  MOTOR_L_INVERT);
  setOne(PIN_ENB, PIN_IN3, PIN_IN4, right, MOTOR_R_INVERT);
}

// L298N: EN = 1 і IN1 = IN2 → обмотки закорочені, швидка зупинка
void motorsBrake() {
  digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW); digitalWrite(PIN_IN4, LOW);
  analogWrite(PIN_ENA, 255);  analogWrite(PIN_ENB, 255);
}

void motorsCoast() {
  analogWrite(PIN_ENA, 0); analogWrite(PIN_ENB, 0);
  digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW); digitalWrite(PIN_IN4, LOW);
}
