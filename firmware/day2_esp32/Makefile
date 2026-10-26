# Збірка й заливка ESP32 з терміналу (arduino-cli). Перевірено: ядро esp32 3.3.x.
#   make setup    — ОДИН РАЗ: ядро ESP32 (~500 МБ, ставте заздалегідь!)
#   make build    — компіляція (без плати)
#   make upload   — компіляція + заливка. Не льє? Затисніть BOOT на ESP32
#   make monitor  — Serial 115200 (вихід: Ctrl+C), команди: h
#   make ide      — compile_commands.json для clangd у VSCodium
#   make zip      — архів коду для PDF-звіту
FQBN   ?= esp32:esp32:esp32
PORT   ?= $(firstword $(wildcard /dev/ttyUSB* /dev/ttyACM*))
SKETCH  = robot
ESP_URL = https://espressif.github.io/arduino-esp32/package_esp32_index.json

.PHONY: setup build upload monitor ide zip

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

ide:
	arduino-cli compile --fqbn $(FQBN) --only-compilation-database --build-path build $(SKETCH)
	ln -sf build/compile_commands.json compile_commands.json

zip:
	zip -qr ../code_archive_sosny_TD.zip $(SKETCH) Makefile README.md
	@echo "../code_archive_sosny_TD.zip"
