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

Получаемый EXE: `cod3pc/out/build/win-amd64-debug/cod3pc.exe`.
SDK автоматически копирует runtime, Tracy и Xenos DLL рядом с EXE.
Приложение по умолчанию выбирает GPU plugin `xenos`.
Пути данных задаются launch-скриптом; не переносите машинные пути в Git.
