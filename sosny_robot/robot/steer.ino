// steer.ino — блок серво шарніра рами (SG90).

Servo steerServo;

void steerSetup() {
  steerServo.attach(PIN_SERVO);
  setSteer(0);
}

// angle: -STEER_MAX (вліво) .. 0 (рівно) .. +STEER_MAX (вправо)
void setSteer(int angle) {
  angle = constrain(angle, -STEER_MAX, STEER_MAX);
  if (SERVO_INVERT) angle = -angle;
  steerServo.write(SERVO_CENTER + angle);
}
