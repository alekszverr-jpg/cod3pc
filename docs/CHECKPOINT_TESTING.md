# Снимки контрольных точек

Снимок сохраняет оригинальные файлы title 415607E1 вместе с header и SHA-256.
Игра должна быть закрыта, чтобы не скопировать незавершённую автозапись.
Скрипт не редактирует содержимое save и не останавливает игру. Существующий
снимок или тестовый профиль не перезаписываются. Restore разрешён только
в новый изолированный debug-профиль; основной прогресс не заменяется.

Для точки конца Saint-Lo нужно один раз пройти до неё в отдельном профиле:

```powershell
# Оригинальное меню и новая игра в отдельном профиле
./tools/Run-Native.ps1 -Seconds 0 -Interactive -DebugProfile saint_lo_end -DofStrength 35
# Дойти до последнего checkpoint перед завершением миссии и штатно закрыть игру.
./tools/Checkpoint.ps1 -Action Save -Name saint_lo_final -DebugProfile saint_lo_end

# Каждый повтор использует новое имя профиля, исходный снимок остаётся целым.
./tools/Checkpoint.ps1 -Action Restore -Name saint_lo_final -DebugProfile saint_lo_repeat_01
./tools/Run-Native.ps1 -Seconds 0 -Interactive -DebugProfile saint_lo_repeat_01 -DofStrength 35
# Выбрать Continue через оригинальное меню.
```

`-DebugProfile` выбирает userdata и оставляет обычное меню/Continue.
`-DebugMission` дополнительно передаёт оригинальный +devmap, поэтому для
восстановления checkpoint использовать именно DebugProfile.

2026-10-15: создан снимок текущей Island `island_before_relaunch` и проверено
восстановление в `island_checkpoint_test`; файлы совпали по SHA-256, повторный
Restore в тот же профиль отвергнут. Основной save не изменён. Снимка конца
Saint-Lo пока нет. Наличие строк checkpoint/loadgame/loadgame_restart в XEX
не доказывает возможность перехода к произвольной точке без прохождения.
Private evidence: logs/ghidra-checkpoint-commands.log.

Снимки находятся под userdata/checkpoints, тестовые профили под
userdata/debug-levels. Все эти файлы приватны и исключены из Git.
