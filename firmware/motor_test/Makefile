# Тест моторів ESP32 з терміналу (arduino-cli), як у firmware/day2_esp32.
#   make setup    — ОДИН РАЗ: ядро ESP32 (якщо ще не ставили для day2_esp32)
#   make upload   — компіляція + заливка. Не льє? Затисніть BOOT на ESP32
#   make monitor  — бачити, який крок зараз іде (вихід: Ctrl+C)
#   make test     — upload + одразу monitor
FQBN   ?= esp32:esp32:esp32
PORT   ?= $(firstword $(wildcard /dev/ttyUSB* /dev/ttyACM*))
SKETCH  = motor_test
ESP_URL = https://espressif.github.io/arduino-esp32/package_esp32_index.json

.PHONY: setup build upload monitor test

setup:
	arduino-cli config init --overwrite || true
	arduino-cli config add board_manager.additional_urls $(ESP_URL)
	arduino-cli core update-index
	arduino-cli core install esp32:esp32

build:
	arduino-cli compile --fqbn $(FQBN) $(SKETCH)

upload:
	@test -n "$(PORT)" || (echo "Плату не знайдено: /dev/ttyUSB* або /dev/ttyACM*"; exit 1)
	arduino-cli compile --upload -p $(PORT) --fqbn $(FQBN) $(SKETCH)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=115200

test: upload monitor
