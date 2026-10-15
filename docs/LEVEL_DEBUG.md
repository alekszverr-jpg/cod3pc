# Отладочный выбор уровня

Режим добавлен 2026-10-14 по запросу пользователя. Он запускает исходную
команду движка devmap и не редактирует campaign save ради разблокировки меню.
Обычный запуск и «Продолжить» используют основной профиль как раньше.

```powershell
# Все оригинальные SP-модули и готовность их native DLL
./tools/Run-Native.ps1 -ListLevels

# Прямой старт Island с отдельным профилем и принятым DoF
./tools/Run-Native.ps1 -Seconds 0 -Interactive -DebugMission island -DofStrength 35

# Просмотр аргументов без запуска и создания профиля
./tools/Run-Native.ps1 -DebugMission island -DofStrength 35 -DryRun
```

Сейчас запускаются `saint_lo` и `island`. Остальные 13 модулей видны в
перечне, но отклоняются до запуска, пока не подключены и не собраны.
Credits отмечены отдельно; алфавитный перечень не является порядком кампании.

Для обычного меню/Continue внутри отдельного userdata используйте
`-DebugProfile <имя>` вместо `-DebugMission`: эта опция не передаёт devmap.
Снимки оригинальных save и повторные прогоны описаны в
[CHECKPOINT_TESTING.md](CHECKPOINT_TESTING.md).
По мере интеграции модулей добавлять их проверенный `debug_map` в manifest:
имя карты не всегда совпадает с именем DLL. Это metadata нашего launch-скрипта.
В Ghidra нашего оригинального default.xex найдены devmap/spmap и буквальные
`+devmap saint_lo` / `+devmap island` (logs/ghidra-map-commands.log).

Профиль debug-миссии располагается в `userdata/debug-levels/<module>/`.
SDK направляет туда userdata, profiles, cache и записи сохранений, если
исходная игра их создаёт. Основной save не редактируется и не копируется
поверх debug-профиля. Возобновление debug checkpoint отдельно пока не проверено.
Чтобы вернуться к основной кампании, запустить без DebugMission и выбрать
«Продолжить». Для второго запуска требуется закрыть предыдущий cod3pc:
скрипт отклоняет дубликат. FirstMission и DebugMission взаимоисключающие.

## Проверка

- Синтетические тесты проверяют запрет запуска несобранного/неправильно
  mapped модуля, явное имя карты и отклонение неизвестных/небезопасных ID.
- ListLevels распознаёт 15 настоящих SP XEX по magic через campaign inventory;
  ready=True только для двух интегрированных DLL с debug_map.
- DryRun Island указывает `+devmap island` и отдельный user_data_root.
  Несобранный nightd отклонён до Start-Process.
- Strict codegen и native/GPU build прошли (level-debug-build-console.log).
  10 parser tests и 2 native tests прошли (level-debug-tests-console.log).
  PowerShell parsing прошёл.
- После разрешённого завершения предыдущего теста открыт один debug-процесс.
  native-20261014-153521-337.log подтвердил оригинальный entry, запрос Island,
  native entry reason=1/result=1 и LOAD_RESULT=0. Уровень с оружием/HUD
  визуально проверен через окно игры. Первый запуск нового профиля выполнял
  загрузку без старого shader cache; начальная пауза в логе не была вылетом.

Debug-режим не доказывает естественный переход между миссиями и не заменяет
проверки AI, checkpoint, прохождения и очистки состояния кампании.
