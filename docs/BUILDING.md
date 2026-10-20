# Сборка Windows x64

VS Code используется как редактор. Установлены отдельные Visual Studio Build
Tools 2022 (17.14.41), MSVC 14.44 и Windows SDK 10.0.26100.0.
Встроенный Clang 19.1.5 оказался ниже рекомендации Wiki; дополнительно установлен
официальный LLVM 23.1.3. Скрипты выбирают его первым. Python 3.12.14 берётся из
локального Codex runtime; при переносе можно использовать обычный Python 3.
Git 2.55.0.windows.3 выбирается явно, вместо MSYS Git из PATH.

Локальные CMake 4.4.4 и Ninja 1.13.2 установлены через pip в
`external/python-tools/`. Официальный SDK v0.10.0 находится в
`external/sdk-v0.10.0/win-amd64/`. Исходники SDK и рекурсивные зависимости:
`external/rexglue-sdk`, commit `c94f5ebdcb3c9d1a460ca48e04f9758448f8d518`.

```powershell
./tools/Check-Environment.ps1
./tools/Build.ps1 -Stage Tests
./tools/Build.ps1 -Stage Analyze
./tools/Build.ps1 -Stage Codegen
./tools/Build.ps1 -Stage Build
./tools/Run-Native.ps1 -Seconds 30 -Headless
```

Инициализация действительно выполнена установленным CLI:

```powershell
./external/sdk-v0.10.0/win-amd64/bin/rexglue.exe init --project-name cod3pc --xex-path ./assets/game/default.xex --game-root ./assets/game --project-root ./cod3pc
```

В v0.10.0 используются `[project]`, `[entrypoint]`, manifest и includes;
старые `--app_name`/`--app_root` из Wiki этим бинарником не поддерживаются.
Относительные входные пути init разрешает от текущей рабочей папки, затем
пересчитывает для папки проекта. Не запускать повторный init поверх изменений.
Документированный [Getting Started](https://github.com/rexglue/rexglue-sdk/wiki/Getting-Started)
используется с обязательной сверкой локального `--help`.

Для сборки SDK: `./tools/Build.ps1 -Stage SDK`. Пресет `win-amd64` подтверждён.
На Windows Git сохранил 15 symlink в libmspack как текст относительного пути;
это вызвало ошибку C-компиляции `expected identifier`. Скрипт
`tools/materialize_mspack.py` безопасно заменяет только эти placeholders
исходным содержимым целей внутри submodule; лицензии сохраняются.

`Build` и `Codegen` используют `--ignore-stamp`. Иначе кеш от `--force`
может обойти повторную валидацию. `Diagnostic` разрешён только для исследования;
его успешный exit code не является доказательством корректности.

По умолчанию Build создаёт оптимизированную сборку с символами:
`cod3pc/out/build/win-amd64-relwithdebinfo/cod3pc.exe` (`-O2`).
`-Configuration Debug` оставляет отдельную сборку без оптимизации (`-O0`);
не используйте её для оценки скорости видео или игрового процесса.
Run-Native выбирает ту же конфигурацию, по умолчанию RelWithDebInfo.
Для визуального теста: `./tools/Run-Native.ps1 -Seconds 600 -Interactive`.
Для проверки штатной команды старта миссии: добавить `-FirstMission`
(передаёт `+spmap saint_lo` в ExLoadedCommandLine через SDK `--cl`).
Этот запуск требует отдельной проверки; при отсутствии автоматического старта
выбирайте миссию через меню.
SDK автоматически копирует runtime, Tracy и Xenos DLL рядом с EXE.
Приложение по умолчанию выбирает GPU plugin `xenos`.
Пути данных задаются launch-скриптом; не переносите машинные пути в Git.

Saint-Lô собирается как отдельная `cod3pc_saint_lo.dll` из manifest [[modules]].
При необработанном Windows exception программа пытается сохранить локальные
`cod3pc-crash.txt` (стек/строки) и `cod3pc-crash.dmp` рядом с EXE. Обработчик
не восстанавливает исполнение и не подавляет ошибку. Файлы остаются в ignored out.

Настройка глубины резкости при запуске (2026-10-14):
`./tools/Run-Native.ps1 -Seconds 0 -Interactive -DofStrength 35`.
Диапазон 0–100: 100 сохраняет исходный DoF (значение по умолчанию),
35 задаёт мягкое смешивание, 0 убирает размытие от этого прохода.
Настройка применяется в исправленном D3D12 plugin xenos-grass;
с -StockGpu или -GpuCoherencyTest нестандартное значение отклоняется.
Для прямого запуска с этим plugin: `--cod3_dof_strength=35`.
Это настройка запуска, пункта в оригинальном игровом меню пока нет.
Изменение силы требует перезапуска; удалять shader cache не нужно.

Для двух DLL кампании с общим image base используется локальный патч SDK
`rexglue-v0100-dll-overlay-groups.patch`. Build-CodegenOverlays.ps1 собирает
CLI из закреплённого исходника 0.10.0; Build.ps1 вызывает его автоматически
для Codegen/Diagnostic/Build. Используется rexgluerd.exe из ignored SDK out,
а CMake получает этот же CLI через COD3_CODEGEN_EXECUTABLE. Установленный
официальный SDK runtime сохраняется. В manifest overlay_group явно задаёт
взаимоисключающие DLL; это расширение нашего проекта, не штатное поле 0.10.0.

Debug-запуск уровня:

```powershell
./tools/Run-Native.ps1 -ListLevels
./tools/Run-Native.ps1 -Seconds 0 -Interactive -DebugMission island -DofStrength 35
```

`ready=True` означает наличие native DLL и проверенного debug_map в manifest.
Сейчас готовы saint_lo и island; остальные XEX показываются в списке и
станут доступны после их интеграции. Credits отмечены отдельно. Имена в
списке не устанавливают порядок миссий. Имя map может отличаться от DLL,
поэтому debug_map задаётся явно и проверяется по оригинальному образу.

DebugMission передаёт исходному движку `+devmap <map>` и использует отдельный
user_data_root: `userdata/debug-levels/<mission>/`. Основной campaign save
не редактируется; autosaves/debug caches остаются в отдельном профиле.
`-DryRun` печатает параметры без запуска и создания профиля. Обычный запуск
без DebugMission остаётся через меню с основным userdata. FirstMission —
старый отдельный параметр; совместно с DebugMission он отклоняется.

Resolve readback обычного Run-Native установлен в `fast`: оригинальная
проверка видимости огоньков получает данные GPU вместо постоянного
просвечивания. Для сравнения: `-ResolveReadback none` (прежний режим) или
`-ResolveReadback full` (немедленное чтение, потенциально дороже).
Старый `-FullGpuReadback` дополнительно синхронизирует memexport; его нельзя
совмещать с явно заданным ResolveReadback, отличным от full. Переключение
`readback_resolve` через F4 возможно в живой диагностической сессии.
