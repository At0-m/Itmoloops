# ITMO Loops

Небольшой консольный рендерер музыки на C++. Читает композицию в DSL ITMO Loops и генерирует mono WAV 44.1 kHz / 16-bit.

## Что умеет

- парсить DSL с `bpm`, `instrument`, `pattern` и вызовами `@pattern`
- поддерживает инструменты `sampler`, `sine`, `square`, `triangle`
- применяет эффекты `gain`, `echo`, `tremolo`
- проверять партитуру без рендера
- сохранять результат в `.wav`

## Сборка

```bash
cmake -S . -B build
cmake --build build
```

## Команды

Сгенерировать `.wav`:

```bash
./build/bin/itmoloops examples/TheRealSlim.txt OUT.wav
```

Проверить файл без рендера:

```bash
./build/itmoloops --check-only SCORE.txt
```

Полезные опции:

```bash
-n, --normalize   нормализовать итоговый сигнал
-v, --verbose     вывести путь к готовому файлу
-h, --help        показать справку
```

Формат вывода фиксированный:

```text
44100 Hz / 16-bit / mono
```

## Примеры

```bash
./build/itmoloops examples/lick.txt lick.wav
./build/itmoloops examples/sweden.txt sweden.wav
./build/itmoloops --check-only examples/lick.txt
```

> Примеры лучше запускать из корня репозитория: в них используются относительные пути к `./samples`.

## Стек

C++17, CMake, GoogleTest