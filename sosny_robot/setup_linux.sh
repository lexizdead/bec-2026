#!/usr/bin/env bash
# setup_linux.sh — один раз на Arch: arduino-cli, ядро AVR, Servo, розширення VSCodium.
# Запуск з папки проєкту:  bash setup_linux.sh   (без sudo; пароль спитає сам)
set -euo pipefail
cd "$(dirname "$0")"
G='\e[32m'; Y='\e[33m'; R='\e[31m'; N='\e[0m'
step() { echo -e "\n${G}==> $*${N}"; }
warn() { echo -e "${Y}!  $*${N}"; }

[[ $EUID -eq 0 ]] && { echo "Запускай БЕЗ sudo"; exit 1; }

step "1/5 Пакети"
if command -v pacman >/dev/null; then
  sudo pacman -S --needed --noconfirm arduino-cli clang zip git
else
  command -v arduino-cli >/dev/null || {
    warn "Не Arch: став arduino-cli так:"
    echo "  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh"
    exit 1; }
fi

step "2/5 Доступ до USB-порту"
GRP=uucp; getent group uucp >/dev/null || GRP=dialout
RELOGIN=0
if id -nG "$USER" | grep -qw "$GRP"; then echo "вже в групі $GRP"
else sudo usermod -aG "$GRP" "$USER"; RELOGIN=1; fi

step "3/5 Ядро arduino:avr + Servo (потрібен інтернет)"
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli lib install Servo

step "4/5 Розширення редактора"
for c in codium code; do
  command -v "$c" >/dev/null && "$c" --install-extension llvm-vs-code-extensions.vscode-clangd --force >/dev/null && echo "$c: clangd ✓"
done

step "5/5 Перевірка збірки"
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old --only-compilation-database --build-path build robot >/dev/null || true
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old robot
echo -e "\n${G}✓ Готово. Відкривай папку в редакторі й тисни Ctrl+Shift+B.${N}"
[[ $RELOGIN -eq 1 ]] && warn "Вийди з системи й зайди знову, інакше заливка дасть Permission denied."
exit 0
