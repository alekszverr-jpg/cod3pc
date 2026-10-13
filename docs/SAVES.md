# Сохранения: диагностика

2026-10-13: после обычного запуска пользователь видит только новую игру,
выбор устройства/предупреждение не появлялись. Продолжение с checkpoint после
закрытия процесса не подтверждено. Проверенный userdata содержит только cache;
профиля и сохранения кампании там нет. Это не устанавливает, на какой стадии
игровая логика отказалась от записи.

Run-Native.ps1 передаёт постоянный --user_data_root в локальную userdata.
В pinned SDK UserProfile::signin_state() возвращает локальный вход для user 0;
XamUserGetSigninState использует этот профиль. ContentManager поддерживает
запись и чтение content packages в userdata/XUID/title/content_type, профильные
settings пишутся UserProfile::SaveSetting. Наличие этих API не доказывает,
что игра успешно вызывает их. В предыдущих info-логах успешные обращения
к storage не видны.

Добавлена диагностика COD3_SAVE_*: original content creation и его возврат,
enumeration, device selector, profile read/write и их возвраты. Hook locations
проверены Ghidra InspectCod3.java по pinned default.xex SHA-256:

| Точка | Инструкция / наблюдение |
|---|---|
| 82344324 / 82344328 | bl 8257899C (XamContentCreateEx), следующая инструкция b 82344330 |
| 82344358 | b 825789DC (XamContentCreateEnumerator) |
| 82343A88 | b 8257890C (XamShowDeviceSelectorUI) |
| 82195EA4 / 82195EA8 | bl 8257887C (XamUserWriteProfileSettings), следующая addi r1,r1,128 |
| 82195E58 / 82195E5C | bl 8257886C (XamUserReadProfileSettings), следующая addi r1,r1,160 |

Инструкционные свидетельства: logs/ghidra-save-trace.log. Generated functions
не редактировались. Hooks только логируют аргументы/возвраты и сохраняют
оригинальные PPC регистры, код и результат; значения буферов не печатаются.
IO_PENDING означает асинхронный запрос, а не подтверждение успешной записи.

Следующая проверка: обычное меню → новая игра → первая контрольная точка →
штатный выход в меню/закрытие → повторный запуск и Continue. Сопоставить
COD3_SAVE_* с фактическими файлами и оригинальной обработкой результатов.
Восстановление checkpoint в текущем процессе отдельно от сохранения на диск.
Не создавать фиктивный save, не подменять меню/успех storage и не переносить
непроверенный формат данных другой игры.

Проверки: Build.ps1 -Stage Tests (восемь tests), Build.ps1 -Stage Build
(strict codegen с --ignore-stamp, Windows RelWithDebInfo) прошли; консольные
логи save-trace-tests-console.log и save-trace-build-console.log.
Первый link выявил несовпадение сигнатур hooks: midasm без registers генерирует
void(), а RAW hook принимает context/base. Исправлено явным списком registers
и соответствующими PPCRegister& параметрами по API pinned codegen.
После исправления сборка прошла. Открыт один обычный игровой запуск;
игровая запись/восстановление ещё требуют ручной проверки.

Лог native-20261013-110503-241.log подтверждает оригинальный entry и чтение
профиля: первый возврат 0x7A (запрос размера буфера), второй 0 (успех).
Игра перечисляет content types 1/2/3 для user 0, device 0. Эти начальные
вызовы работают, но не доказывают запись checkpoint. Дальнейшее наблюдение
нужно после новой игры/первого checkpoint и штатного выхода.

После ручной проверки пользователь нажал «Выйти», процесс завершился.
native-20261013-110503-241.log: content creation flags 0x12 возвращает 0;
завершение Execution complete в 11:22:24.821, FATAL отсутствует. На диске
появились оригинальный payload
userdata/B13EBABEBABEBABE/415607E1/00000001/COD3_SaveSPV1.0/COD3_SaveSPV1.0
(6928 байт), header (328 байт) и achievements/415607E1.toml.
Подтверждена реальная запись, а не только успешный storage return.
Причина отсутствия файлов в предыдущих запусках пока не установлена.
Новый обычный запуск подготовлен для проверки Continue и восстановления
checkpoint; наличие payload само по себе не подтверждает правильный прогресс.
