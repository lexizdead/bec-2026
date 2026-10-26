#include "magazine.h"
#include "config.h"
#include <Servo.h>

static Servo servo;
static uint8_t pos = 0;      // індекс у MAG_ANGLES
static int angle = 0;        // поточний кут

static void moveTo(int target) {
  // плавно: менше ривків по живленню і шкант не підкидає
  while (angle != target) {
    if (abs(target - angle) <= MAG_STEP_DEG) angle = target;
    else angle += (target > angle) ? MAG_STEP_DEG : -MAG_STEP_DEG;
    servo.write(angle);
    delay(MAG_STEP_MS);
  }
}

void magBegin() {
  servo.attach(PIN_SERVO, MAG_PULSE_MIN, MAG_PULSE_MAX);
  angle = MAG_ANGLES[0];
  servo.write(angle);
  pos = 0;
}

void magHome() {
  moveTo(MAG_ANGLES[0]);
  pos = 0;
}

uint8_t magDropped() { return pos; }
bool magEmpty() { return pos >= N_DOWELS; }

void magDropNext() {
  if (magEmpty()) return;
  moveTo(MAG_ANGLES[pos + 1]);
  pos++;
  delay(MAG_DROP_WAIT_MS);
}
