# Снимки контрольных точек

2026-10-19: проверка поздней Island. Снимок nightd_after_island_01 содержит
строки ISLAND/i_event_6, но пользователь подтвердил: Continue выбирает Night
Drop. Имя checkpoint внутри файла не доказывает возможность его повторной
загрузки после отметки о завершении миссии. Остальные оригинальные копии
содержат i_event_1 либо i_event_2; проверенного финального Island до completion
среди них нет. Для теста Island → Night Drop нужен именно конец старой миссии.

Ghidra по текущему E057 XEX: loadgame зарегистрирован на 82518580,
loadgame_restart на 82518518; loadgame_continue использует tail branch
82515030 → 82514530. Первые два читают обычные SVG-файлы, формат имени
save/autosave/%s.svg берётся из 820695C4. При прямом +loadgame autosave/island.svg
в отдельном island_checkpoint_direct_01 лог native-20261019-090857-599.log
показывает NO_SUCH_FILE для d:\save\autosave\island.svg. Оригинальный Xbox
сейв в content package не преобразовывался в SVG. Экспериментальный флаг runner
удалён после неуспеха; рабочий launcher остался прежним. spmap читает карту,
но проверенный обработчик не принимает отдельный checkpoint аргумент.
Private evidence: ghidra-loadgame-registration.log, ghidra-loadgame-handlers.log,
ghidra-checkpoint-restore-path.log, ghidra-spmap-checkpoint-args.log,
ghidra-spmap-load-state.log. ReadCod3Strings.java — read-only вспомогательный
скрипт; вывод оригинальных строк хранится только в приватных logs.

Подтверждение 2026-10-15: из копии saint_lo_final пользователь завершил
Saint-Lo и после native relaunch получил управляемую Island. Снимок пригоден
для повторения этого перехода; дальнейшая автозапись затрагивает только
saint_lo_repeat_01, не исходный снимок и не основной userdata.

2026-10-15: после сообщения пользователя «Готово» сохранён настоящий профиль
saint_lo_end в снимок saint_lo_final (save 6928 байт, header 328 байт).
Файлы проверены по SHA-256; восстановлены в saint_lo_repeat_01 без изменения
снимка и основного Island. Этот профиль открыт через обычное меню для проверки
места Continue и перехода; точное положение точки ещё ожидает подтверждения.

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
