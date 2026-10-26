mkdir -p ~/sosny_test/motor_sonar_test && cat > ~/sosny_test/motor_sonar_test/motor_sonar_test.ino <<'EOF'
// sosny — мотор 1 с вперед, 1 с назад, стоп; потім передній HC-SR04.
// Мотор: L298N OUT3/OUT4 -> ENB D6, IN3 D11, IN4 D12 (перемичку ENB зняти).
// Датчик: TRIG A2, ECHO A3, VCC 5V, GND. Світлодіод L = ближче 20 см.
const int ENB = 6, IN3 = 11, IN4 = 12;
const int TRIG = A2, ECHO = A3;

void motor(int spd) {
  digitalWrite(IN3, spd > 0);
  digitalWrite(IN4, spd < 0);
  analogWrite(ENB, abs(spd));
}

int distanceCm() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long us = pulseIn(ECHO, HIGH, 25000);
  if (us == 0) return -1;
  return us / 58;
}

void setup() {
  Serial.begin(9600);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  motor(0);
  delay(2000);
  Serial.println("Motor forward"); motor(200);  delay(1000);
  Serial.println("Motor back");    motor(-200); delay(1000);
  Serial.println("Stop");          motor(0);
  Serial.println("Sensor test:");
}

void loop() {
  int d = distanceCm();
  if (d < 0) Serial.println("no echo: check TRIG/ECHO, 5V, GND");
  else { Serial.print(d); Serial.println(" cm"); }
  digitalWrite(LED_BUILTIN, d >= 0 && d < 20);
  delay(200);
}
EOF
export PATH="$HOME/.local/bin:$PATH"
if ! command -v arduino-cli >/dev/null; then
  if command -v pacman >/dev/null; then sudo pacman -S --needed --noconfirm arduino-cli
  else mkdir -p ~/.local/bin && curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh; fi
fi
arduino-cli core update-index && arduino-cli core install arduino:avr
PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1)
if [ -z "$PORT" ]; then echo "!!! Плату не видно: інший кабель / USB"; else
  [ -w "$PORT" ] || sudo chmod a+rw "$PORT"
  echo ">>> Плата на $PORT"
  arduino-cli compile -u -p "$PORT" --fqbn arduino:avr:nano:cpu=atmega328old ~/sosny_test/motor_sonar_test \
   || arduino-cli compile -u -p "$PORT" --fqbn arduino:avr:nano:cpu=atmega328 ~/sosny_test/motor_sonar_test \
   && arduino-cli monitor -p "$PORT" -c baudrate=9600
fi
