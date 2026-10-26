#!/usr/bin/env bash
# ======================================================================
#  setup_sosny.sh — готує Arch + VSCodium до роботи з шаблоном прошивки
#  Запуск:  bash setup_sosny.sh [шлях/до/robot_template.zip] [папка]
#  За замовчуванням: zip шукає в ~/Downloads, ставить у ~/sosny/firmware
#  Можна запускати повторно — нічого не зламає, існуючий код не перезапише.
# ======================================================================
set -euo pipefail

ZIP="${1:-$HOME/Downloads/robot_template.zip}"
DEST="${2:-$HOME/sosny}"
G='\e[32m'; Y='\e[33m'; R='\e[31m'; N='\e[0m'
step() { echo -e "\n${G}==> $*${N}"; }
warn() { echo -e "${Y}!  $*${N}"; }
die()  { echo -e "${R}✗  $*${N}"; exit 1; }

[[ $EUID -eq 0 ]] && die "Запускай БЕЗ sudo (пароль спитає сам, коли треба)."
command -v pacman >/dev/null || die "Це не Arch (немає pacman)."

# ---------- 1. Пакети ----------
step "1/7 Пакети: arduino-cli clang make zip unzip git"
sudo pacman -S --needed --noconfirm arduino-cli clang make zip unzip git

# ---------- 2. Доступ до порту ----------
step "2/7 Доступ до USB-порту (група uucp)"
RELOGIN=0
if id -nG "$USER" | grep -qw uucp; then
  echo "вже в групі uucp"
else
  sudo usermod -aG uucp "$USER"
  RELOGIN=1
  warn "Додано в uucp — ПІСЛЯ скрипта вийди з системи й зайди знову (або перезавантаж)."
fi

# ---------- 3. Ядро Arduino ----------
step "3/7 Ядро arduino:avr + Servo (потрібен інтернет)"
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli lib install Servo

# ---------- 4. Шаблон ----------
step "4/7 Шаблон → $DEST/firmware"
mkdir -p "$DEST"
if [[ -f "$DEST/firmware/robot/robot.ino" ]]; then
  warn "$DEST/firmware вже існує — не чіпаю, щоб не затерти твій код."
else
  [[ -f "$ZIP" ]] || die "Не знайдено $ZIP. Вкажи шлях: bash setup_sosny.sh /шлях/robot_template.zip"
  unzip -q "$ZIP" -d "$DEST"
  echo "розпаковано"
fi
cd "$DEST/firmware"

# ---------- 5. Git ----------
step "5/7 Git"
if [[ ! -d .git ]]; then
  git init -q
  git config user.name  >/dev/null 2>&1 || git config user.name  "sosny"
  git config user.email >/dev/null 2>&1 || git config user.email "sosnyqq@gmail.com"
  git add . && git commit -qm "шаблон" && echo "репозиторій створено"
else
  echo "репозиторій уже є"
fi

# ---------- 6. VSCodium ----------
step "6/7 Розширення VSCodium"
CODIUM=""
for c in codium vscodium; do command -v "$c" >/dev/null && { CODIUM="$c"; break; }; done
if [[ -z "$CODIUM" ]]; then
  warn "VSCodium не знайдено. Постав: yay -S vscodium-bin (або paru), потім запусти скрипт ще раз."
else
  "$CODIUM" --install-extension llvm-vs-code-extensions.vscode-clangd --force >/dev/null && echo "clangd ✓"
  if "$CODIUM" --list-extensions | grep -qi '^ms-vscode.cpptools$'; then
    "$CODIUM" --uninstall-extension ms-vscode.cpptools >/dev/null && echo "прибрано MS C/C++ (конфліктує з clangd)"
  fi
fi

# ---------- 7. Перевірка збірки ----------
step "7/7 Перевірка: make ide + make build"
make ide   >/dev/null 2>&1 && echo "автодоповнення (compile_commands.json) ✓" || warn "make ide не вдався — не критично"
if make build; then
  echo -e "\n${G}✓ Усе готово. Шаблон збирається.${N}"
else
  die "make build не пройшов — скинь вивід вище Claude."
fi

echo
echo "Порт Arduino: $(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo 'не підключено')"
[[ $RELOGIN -eq 1 ]] && warn "НЕ ЗАБУДЬ: вийди з системи й зайди знову, інакше upload дасть Permission denied."
echo "Відкрити проєкт:  $CODIUM $DEST/firmware"
if [[ -n "$CODIUM" ]]; then
  read -rp "Відкрити зараз? [Y/n] " a || a=n
  [[ "${a,,}" != n ]] && "$CODIUM" "$DEST/firmware" >/dev/null 2>&1 &
fi
exit 0
