#!/usr/bin/env bash
# sosny — Т-150: прості прошивки під кожен вузол. Піни як на schematic/t150_wiring.svg:
#   мотор (один) L298N OUT3/OUT4: ENB D6, IN3 D11, IN4 D12   (перемичку ENB зняти)
#   HC-SR04 лівий  TRIG A0 ECHO A1 | передній TRIG A2 ECHO A3 | правий TRIG A4 ECHO A5
#   серво рами D9 (живлення від DC-DC 5 В) | кнопка Start D2 і GND
#
# Запуск:  bash ~/sosny_test/t150_tests.sh <що>
#   motor   мотор 1 с вперед, 1 с назад, стоп
#   left    лівий датчик        front  передній        right  правий
#   sonars  усі три датчики разом
#   servo   серво: центр -> вліво -> центр -> вправо -> центр (по колу)
#   button  кнопка Start
#   robot   робот: Start -> їде прямо, перешкода спереду < 20 см -> стоп
# Вийти з монітора: Ctrl+C
set -e
WHAT="${1:-}"
DIR="$HOME/sosny_test/t150_$WHAT"
SK="$DIR/t150_$WHAT.ino"
mkdir -p "$DIR"

sonar_code() {   # $1 = назва, $2 = TRIG, $3 = ECHO
cat <<EOF
// sosny — тест датчика: $1 (TRIG $2, ECHO $3). Світлодіод L = ближче 20 см.
const int TRIG = $2, ECHO = $3;
void setup() {
  Serial.begin(9600);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT); pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Sensor: $1");
}
void loop() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long us = pulseIn(ECHO, HIGH, 25000);
  if (us == 0) { Serial.println("no echo: check TRIG/ECHO, 5V, GND"); digitalWrite(LED_BUILTIN, LOW); }
  else { int d = us / 58; Serial.print(d); Serial.println(" cm"); digitalWrite(LED_BUILTIN, d < 20); }
  delay(200);
}
EOF
}

case "$WHAT" in
motor) cat > "$SK" <<'EOF'
// sosny — тест мотора: 1 с вперед, 1 с назад, стоп. Повтор — кнопка RESET на платі.
const int ENB = 6, IN3 = 11, IN4 = 12;
void motor(int spd) { digitalWrite(IN3, spd > 0); digitalWrite(IN4, spd < 0); analogWrite(ENB, abs(spd)); }
void setup() {
  Serial.begin(9600);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  motor(0); delay(2000);
  Serial.println("forward"); motor(200);  delay(1000);
  Serial.println("back");    motor(-200); delay(1000);
  Serial.println("stop");    motor(0);
}
void loop() {}
EOF
;;
left)  sonar_code "LEFT"  A0 A1 > "$SK" ;;
front) sonar_code "FRONT" A2 A3 > "$SK" ;;
right) sonar_code "RIGHT" A4 A5 > "$SK" ;;
sonars) cat > "$SK" <<'EOF'
// sosny — усі три HC-SR04 по черзі. -1 = датчик не відповідає.
const int TRIG[3] = {A0, A2, A4};
const int ECHO[3] = {A1, A3, A5};
int cm(int i) {
  digitalWrite(TRIG[i], LOW);  delayMicroseconds(2);
  digitalWrite(TRIG[i], HIGH); delayMicroseconds(10);
  digitalWrite(TRIG[i], LOW);
  long us = pulseIn(ECHO[i], HIGH, 25000);
  return us ? us / 58 : -1;
}
void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 3; i++) { pinMode(TRIG[i], OUTPUT); pinMode(ECHO[i], INPUT); }
  Serial.println("LEFT\tFRONT\tRIGHT  (cm)");
}
void loop() {
  int l = cm(0); delay(30);
  int f = cm(1); delay(30);
  int r = cm(2); delay(30);
  Serial.print(l); Serial.print('\t'); Serial.print(f); Serial.print('\t'); Serial.println(r);
  delay(150);
}
EOF
;;
servo) cat > "$SK" <<'EOF'
// sosny — тест серво рами (D9). Тяга: рама ±30° = серво ±45° від центру.
// Якщо рама в центрі стоїть криво — підбери CENTER (85, 95...).
#include <Servo.h>
const int CENTER = 90, SWING = 45;
Servo s;
void go(int a) { Serial.print("servo "); Serial.println(a); s.write(a); delay(1500); }
void setup() { Serial.begin(9600); s.attach(9); go(CENTER); }
void loop() { go(CENTER - SWING); go(CENTER); go(CENTER + SWING); go(CENTER); }
EOF
;;
button) cat > "$SK" <<'EOF'
// sosny — тест кнопки Start (D2 і GND). Світлодіод L горить, поки натиснута.
void setup() { Serial.begin(9600); pinMode(2, INPUT_PULLUP); pinMode(LED_BUILTIN, OUTPUT); }
void loop() {
  bool pressed = digitalRead(2) == LOW;
  digitalWrite(LED_BUILTIN, pressed);
  Serial.println(pressed ? "PRESSED" : "-");
  delay(100);
}
EOF
;;
robot) cat > "$SK" <<'EOF'
// sosny — Т-150: Start -> їде прямо (рама рівно); перешкода спереду < 20 см -> стоп,
// вільно > 30 см -> їде далі. Світлодіод L: блимає = чекає Start, горить = їде, гасне = стоїть.
#include <Servo.h>
const int ENB = 6, IN3 = 11, IN4 = 12;
const int TRIG = A2, ECHO = A3;     // передній датчик
const int START = 2, SERVO_PIN = 9;
const int SPEED = 170, STOP_CM = 20, GO_CM = 30, CENTER = 90;
Servo s;
bool moving = false;
void motor(int spd) { digitalWrite(IN3, spd > 0); digitalWrite(IN4, spd < 0); analogWrite(ENB, abs(spd)); }
int cm() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long us = pulseIn(ECHO, HIGH, 25000);
  return us ? us / 58 : 400;        // нема відлуння = попереду вільно
}
void setup() {
  Serial.begin(9600);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(START, INPUT_PULLUP); pinMode(LED_BUILTIN, OUTPUT);
  motor(0);
  s.attach(SERVO_PIN); s.write(CENTER);
  Serial.println("press Start");
  while (digitalRead(START) == HIGH) { digitalWrite(LED_BUILTIN, (millis() / 300) % 2); delay(10); }
  delay(500);
}
void loop() {
  int d = cm();
  if (d < STOP_CM) moving = false;
  else if (d > GO_CM) moving = true;
  motor(moving ? SPEED : 0);
  digitalWrite(LED_BUILTIN, moving);
  Serial.print(d); Serial.println(moving ? " cm GO" : " cm STOP");
  delay(60);
}
EOF
;;
*) echo "Вкажи, що тестувати: motor | left | front | right | sonars | servo | button | robot"; exit 1 ;;
esac

export PATH="$HOME/.local/bin:$PATH"
if ! command -v arduino-cli >/dev/null; then
  if command -v pacman >/dev/null; then sudo pacman -S --needed --noconfirm arduino-cli
  else mkdir -p ~/.local/bin && curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh; fi
fi
arduino-cli core list | grep -q "arduino:avr" || { arduino-cli core update-index; arduino-cli core install arduino:avr; }
arduino-cli lib list | grep -q "^Servo " || arduino-cli lib install Servo

PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1 || true)
[ -z "$PORT" ] && { echo "!!! Плату не видно: інший кабель / USB"; exit 1; }
[ -w "$PORT" ] || sudo chmod a+rw "$PORT"
echo ">>> $WHAT -> $PORT"
arduino-cli compile -u -p "$PORT" --fqbn arduino:avr:nano:cpu=atmega328old "$DIR" \
 || arduino-cli compile -u -p "$PORT" --fqbn arduino:avr:nano:cpu=atmega328 "$DIR"
echo ">>> Залито. Монітор 9600, вийти Ctrl+C"
arduino-cli monitor -p "$PORT" -c baudrate=9600
