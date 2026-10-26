mkdir -p ~/sosny_test && cat > ~/sosny_test/upload.sh <<'SOSNY'
#!/usr/bin/env bash
# sosny: пише скетчі, ставить arduino-cli, компілює, заливає в Arduino Nano, відкриває Serial Monitor.
#   bash ~/sosny_test/upload.sh        -> тест датчика й мотора (без кнопки)
#   bash ~/sosny_test/upload.sh t150   -> t150_test (кнопка Start, світлодіод L, чорний ящик)
set -e
DIR="$HOME/sosny_test"
SKETCH=sensor_motor_test; [ "$1" = "t150" ] && SKETCH=t150_test
mkdir -p "$DIR/sensor_motor_test" "$DIR/t150_test"

# ================= 1. Тест датчика й мотора, без кнопки =================
cat > "$DIR/sensor_motor_test/sensor_motor_test.ino" <<'EOF'
// sosny — тест переднього датчика і мотора, БЕЗ кнопки.
// Мотор: L298N OUT3/OUT4 (ENB D6, IN3 D11, IN4 D12, перемичку ENB зняти).
// HC-SR04: TRIG A2, ECHO A3, VCC 5V, GND.
// 3 с пауза -> мотор 2 с вперед -> 1 с стоп -> 2 с назад -> стоп.
// Далі: крутиться вперед, якщо вільно; стоїть, якщо ближче 20 см. Світлодіод L = перешкода.
const int ENB = 6, IN3 = 11, IN4 = 12;
const int TRIG = A2, ECHO = A3;
const int SPEED = 170;      // 0..255
const int STOP_CM = 20;

int distanceCm() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long us = pulseIn(ECHO, HIGH, 25000);
  if (us == 0) return 400;   // 400 = нічого не бачить або датчик не підключений
  return us / 58;
}

void motor(int spd) {        // + вперед, - назад, 0 стоп
  digitalWrite(IN3, spd > 0);
  digitalWrite(IN4, spd < 0);
  analogWrite(ENB, abs(spd));
}

void setup() {
  Serial.begin(9600);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  motor(0);
  delay(3000);
  Serial.println("Motor forward"); motor(SPEED);  delay(2000);
  Serial.println("Stop");          motor(0);      delay(1000);
  Serial.println("Motor back");    motor(-SPEED); delay(2000);
  Serial.println("Stop");          motor(0);      delay(1000);
  Serial.println("Sensor test: put your hand in front");
}

void loop() {
  int d = distanceCm();
  bool obstacle = d < STOP_CM;
  motor(obstacle ? 0 : SPEED);
  digitalWrite(LED_BUILTIN, obstacle);
  Serial.print(d);
  Serial.println(obstacle ? " cm  OBSTACLE" : " cm");
  delay(100);
}
EOF

# ================= 2. t150_test: Start, світлодіод L, чорний ящик =================
cat > "$DIR/t150_test/t150_test.ino" <<'EOF'
// sosny — Т-150, тест: їде прямо, перешкода спереду -> стоп.
// Мотор на L298N OUT3/OUT4 (перемичку ENB зняти).
// Світлодіод L: повільно блимає = чекає Start, горить = їде, часто блимає = стоїть.
// Чорний ящик: кожні 100 мс пише в EEPROM відстань і стан (~50 с).
// Після заїзду: USB -> Serial Monitor 9600 -> надіслати  d  -> таблиця. Стирається при наступному Start.
#include <Servo.h>
#include <EEPROM.h>

const int ENB = 6, IN3 = 11, IN4 = 12;   // мотор
const int SERVO_PIN = 9;                 // серво рами
const int TRIG = A2, ECHO = A3;          // передній HC-SR04
const int START = 2;                     // кнопка на GND

const bool USE_BUTTON = true;   // кнопки немає -> false (поїде через 3 с після ввімкнення)
const int  SPEED   = 170;       // 0..255
const int  STOP_CM = 20;        // ближче -> стоп
const int  GO_CM   = 30;        // далі -> знову їде
const int  SERVO_CENTER = 90;   // рама рівно
const unsigned long LOG_MS = 100;

Servo servo;
bool moving = false;
int  logPos = 0;
unsigned long lastLog = 0;

int distanceCm() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long us = pulseIn(ECHO, HIGH, 25000);
  if (us == 0) return 400;
  return us / 58;
}

void forward()   { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  analogWrite(ENB, SPEED); }
void stopMotor() { digitalWrite(IN3, HIGH); digitalWrite(IN4, HIGH); analogWrite(ENB, 255); }  // гальмо

void logClear() { EEPROM.update(0, 0xFF); EEPROM.update(1, 0xFF); logPos = 0; }

void logWrite(int d, bool go) {
  if (logPos + 1 >= (int)EEPROM.length()) return;
  EEPROM.update(logPos,     min(d / 2, 254));
  EEPROM.update(logPos + 1, go ? 1 : 0);
  logPos += 2;
  if (logPos + 1 < (int)EEPROM.length()) EEPROM.update(logPos, 0xFF);
}

void logDump() {
  Serial.println(F("time_s\tdist_cm\tstate"));
  for (int i = 0; i + 1 < (int)EEPROM.length(); i += 2) {
    byte d = EEPROM.read(i);
    if (d == 0xFF) break;
    Serial.print((i / 2) * LOG_MS / 1000.0, 1); Serial.print('\t');
    Serial.print(d * 2);                        Serial.print('\t');
    Serial.println(EEPROM.read(i + 1) ? F("GO") : F("STOP"));
  }
  Serial.println(F("--- end ---"));
}

void waitStart() {
  Serial.println(F("Waiting for Start. Send d = dump log"));
  unsigned long t0 = millis();
  while (true) {
    digitalWrite(LED_BUILTIN, (millis() / 500) % 2);
    if (Serial.available() && Serial.read() == 'd') logDump();
    if (USE_BUTTON && digitalRead(START) == LOW) break;
    if (!USE_BUTTON && millis() - t0 > 3000) break;
    delay(10);
  }
  logClear();
  delay(500);
}

void setup() {
  Serial.begin(9600);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(START, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  stopMotor();
  servo.attach(SERVO_PIN);
  servo.write(SERVO_CENTER);
  waitStart();
}

void loop() {
  int d = distanceCm();
  if (d < STOP_CM) moving = false;
  else if (d > GO_CM) moving = true;
  if (moving) forward(); else stopMotor();
  digitalWrite(LED_BUILTIN, moving ? HIGH : (millis() / 100) % 2);
  if (millis() - lastLog >= LOG_MS) { lastLog = millis(); logWrite(d, moving); }
  Serial.print(d);
  Serial.println(moving ? " cm  GO" : " cm  STOP");
  delay(40);
}
EOF

# ================= 3. arduino-cli + ядро AVR =================
export PATH="$HOME/.local/bin:$PATH"
if ! command -v arduino-cli >/dev/null; then
  if command -v pacman >/dev/null; then
    sudo pacman -S --needed --noconfirm arduino-cli
  else
    mkdir -p "$HOME/.local/bin"
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$HOME/.local/bin" sh
  fi
fi
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli lib install Servo

# ================= 4. Компіляція =================
echo ">>> Компілюю $SKETCH"
arduino-cli compile --fqbn arduino:avr:nano "$DIR/$SKETCH"

# ================= 5. Порт =================
PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1 || true)
if [ -z "$PORT" ]; then
  echo "!!! Плату не видно. Інший кабель (з даними) / інший USB. На Ubuntu: sudo apt remove brltty"
  exit 1
fi
[ -w "$PORT" ] || sudo chmod a+rw "$PORT"
echo ">>> Плата на $PORT"

# ================= 6. Заливка: спершу Old Bootloader, потім новий =================
arduino-cli compile -u -p "$PORT" --fqbn arduino:avr:nano:cpu=atmega328old "$DIR/$SKETCH" \
 || arduino-cli compile -u -p "$PORT" --fqbn arduino:avr:nano:cpu=atmega328 "$DIR/$SKETCH"

# ================= 7. Serial Monitor (вийти: Ctrl+C) =================
echo ">>> Залито. Serial Monitor 9600, вийти Ctrl+C"
arduino-cli monitor -p "$PORT" -c baudrate=9600
SOSNY
bash ~/sosny_test/upload.sh
