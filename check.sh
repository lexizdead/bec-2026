#!/usr/bin/env bash
# check.sh — перевіряє, що всі файли репозиторію на місці й не пошкоджені.
# Запуск з папки bec-2026:  bash check.sh
cd "$(dirname "$0")"
if sha256sum --quiet -c MANIFEST.sha256; then
  echo "OK: усі $(wc -l < MANIFEST.sha256) файлів на місці"
else
  echo "!!! Деяких файлів немає або вони змінені (див. вище)"
  exit 1
fi
