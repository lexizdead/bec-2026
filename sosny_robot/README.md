# sosny robot — Arduino Nano

Ті самі файли працюють на Linux (VSCodium) і Windows (VS Code).

## Один раз
- **Linux:** `bash setup_linux.sh`
- **Windows:** подвійний клік `setup_windows.bat`, потім перезапустити VS Code

Відкрити **всю папку `sosny_robot`** у редакторі (File → Open Folder).

## Кнопки
| Що | Як |
|---|---|
| Збірка | `Ctrl+Shift+B` |
| Залити / монітор / архів | `Ctrl+Shift+P` → `Tasks: Run Task` → вибрати |

Порт шукається сам. Не знайшов або знайшов не той — впиши в `.vscode/settings.json`:
`"sosny.port": "COM5"` (Windows) або `"/dev/ttyUSB0"` (Linux).

## Код
```
robot/
  robot.ino   setup() і loop() — тільки виклики блоків
  config.h    ВСІ піни й числа
  motors.ino  блок: мотори
  steer.ino   блок: серво рами
  sonar.ino   блок: далекомір
  avoid.ino   блок: об'їзд
```
**Новий блок** = новий файл `назва.ino` у `robot/`. Його функції одразу видно в `robot.ino`,
нічого підключати не треба. Глобальні змінні блоку тримай у його ж файлі.

Помилки показує збірка (`Ctrl+Shift+B`), а не підсвітка в редакторі.

## Ділимося кодом
Один репозиторій на GitHub: `git pull` перед роботою, `git commit` + `git push` після.
Кожен пише свій блок у своєму файлі — так не буде конфліктів.
