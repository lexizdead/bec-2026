# setup_windows.ps1 — один раз на Windows: arduino-cli, ядро AVR, Servo, розширення VS Code.
# Запуск: подвійний клік по setup_windows.bat (він запускає цей файл).
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
function Step($t) { Write-Host "`n==> $t" -ForegroundColor Green }
function RefreshPath { $env:Path = [Environment]::GetEnvironmentVariable('Path','Machine') + ';' + [Environment]::GetEnvironmentVariable('Path','User') }

Step '1/5 arduino-cli'
if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
  winget install --id ArduinoSA.CLI -e --accept-source-agreements --accept-package-agreements
  RefreshPath
}
if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
  Write-Host 'arduino-cli не знайдено в PATH. Закрий це вікно, запусти setup_windows.bat ще раз.' -ForegroundColor Red
  exit 1
}
arduino-cli version

Step '2/5 Git (щоб ділитися кодом)'
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
  winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
  RefreshPath
} else { Write-Host 'git вже є' }

Step '3/5 Ядро arduino:avr + Servo (потрібен інтернет)'
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli lib install Servo

Step '4/5 Розширення VS Code'
if (Get-Command code -ErrorAction SilentlyContinue) {
  code --install-extension llvm-vs-code-extensions.vscode-clangd --force | Out-Null
  Write-Host 'clangd OK'
} else { Write-Host 'VS Code не знайдено в PATH — постав розширення clangd вручну' -ForegroundColor Yellow }

Step '5/5 Перевірка збірки'
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old --only-compilation-database --build-path build robot | Out-Null
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old robot
if ($LASTEXITCODE -ne 0) { Write-Host 'Збірка не пройшла — скинь вивід Claude.' -ForegroundColor Red; exit 1 }

Write-Host "`nГотово. ПЕРЕЗАПУСТИ VS Code (щоб побачив arduino-cli), відкрий цю папку, тисни Ctrl+Shift+B." -ForegroundColor Green
Write-Host 'Плата не видна в COM-портах? Постав драйвер CH340 (китайські Nano).' -ForegroundColor Yellow
