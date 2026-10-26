.RECIPEPREFIX := >
FQBN ?= esp32:esp32:esp32
PORT ?= $(firstword $(wildcard /dev/ttyUSB* /dev/ttyACM*))
.PHONY: upload monitor test
upload:
> @test -n "$(PORT)" || (echo "Плату не знайдено"; exit 1)
> arduino-cli compile --upload -p $(PORT) --fqbn $(FQBN) drive_test
monitor:
> arduino-cli monitor -p $(PORT) -c baudrate=115200
test: upload monitor
