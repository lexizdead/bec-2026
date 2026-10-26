# Збірка й заливка з терміналу (arduino-cli). Те саме можна робити кнопками в Arduino IDE.
#   make setup    — один раз: ядро AVR + бібліотека Servo
#   make build    — компіляція (працює і без підключеної плати)
#   make upload   — компіляція + заливка
#   make monitor  — Serial-монітор 115200 (вихід: Ctrl+C)
#   make ide      — compile_commands.json для clangd у VSCodium
#   make zip      — архів коду для PDF-звіту
#
# Китайські Nano з CH340 майже завжди мають старий загрузчик (atmega328old).
# Якщо upload пише "not in sync" — спробуйте: make upload CPU=atmega328
CPU    ?= atmega328old
FQBN   ?= arduino:avr:nano:cpu=$(CPU)
PORT   ?= $(firstword $(wildcard /dev/ttyUSB* /dev/ttyACM*))
SKETCH  = kabel_ukladach

.PHONY: setup build upload monitor ide zip

setup:
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install Servo

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
	zip -qr ../code_archive.zip $(SKETCH) Makefile README.md
	@echo "../code_archive.zip"
