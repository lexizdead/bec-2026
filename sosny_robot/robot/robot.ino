// robot.ino — головний файл: тільки setup() і loop().
// Кожен блок — окремий файл-вкладка в цій же папці (motors.ino, steer.ino, ...).
// Arduino склеює всі .ino в один, тому функції з будь-якого блоку видно звідусіль.
//
// Зараз: їдемо прямо (рама рівно). Перешкода ближче STOP_CM -> стоп.
// Перешкоду прибрали (далі GO_CM) -> їдемо далі.

#include <Servo.h>
#include "config.h"

bool moving = false;

void setup() {
  Serial.begin(SERIAL_BAUD);
  motorsSetup();
  steerSetup();          // рама рівно
  sonarSetup();
  pinMode(PIN_START, INPUT_PULLUP);

  Serial.println(F("sosny: press Start"));
  while (digitalRead(PIN_START) == HIGH) delay(10);
  delay(500);
}

void loop() {
  int d = sonarCm();

  if (d < STOP_CM)      moving = false;   // перешкода -> стоп
  else if (d > GO_CM)   moving = true;    // вільно -> їдемо
  // між STOP_CM і GO_CM нічого не міняємо

  setSteer(0);
  if (moving) drive(SPEED_FWD);
  else        stopAll();

  Serial.print(d); Serial.println(moving ? F(" cm  GO") : F(" cm  STOP"));
  delay(20);
}
