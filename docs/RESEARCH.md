# Исследование Call of Duty 3 Xbox 360

Дата: 2026-10-12, Asia/Yekaterinburg. Источник: локальная копия пользователя.

## Проверенные данные

GoD header `415607E1/00007000/2E97F145D2D59518C2F4`, magic LIVE,
SVOD type 1, 37 фрагментов, XSF layout, GDFX magic в Data0000 по `0x12000`.
552 файла; 6 190 037 625 байт извлечённого содержимого. Использован только
локальный reader rexauto commit `dac49b331f2632f0b050f469e35747721300b8b2`.
Полная rexauto pipeline, remote cures и cloud builds не запускались.
Поддержка GoD проверена на этой копии, поддержка CoD3 как игры не установлена.
[Исходный проект rexauto](https://github.com/Player124413/rexauto).

| Свойство | default.xex |
|---|---|
| Размер | 7 254 016 байт |
| SHA-256 | 2944eec7d1231ad6798b5f9f8adf8855f5e489296b22eab45b27a577cee23692 |
| Title ID / Media ID | 415607E1 / 2E07093A |
| Version / Base version | 0.0.0.1 / 0.0.0.1 |
| Region mask | FFFFFFFF |
| Entry / generated symbol | 82344D00 / xstart |
| Image base / size | 82000000 / 00C30000 |
| Image payload offset | 00003000 |
| Encryption / compression fields | 1 / 1 |
| File timestamp (SDK) | 2006-10-10 02:01:39 UTC |

Title Update не установлен независимо: номер TU не выводится из version=1.
`default.xexp` отсутствует по runtime VFS. CoD Xe адреса MP TU3 не применялись.

`codmp_xenonf.xex`: 7 122 944 байта,
SHA-256 `bd57d0df66172ed58163fcfb8a814640dddda3dac07ce2d6b79bf226b20982f6`.
Кроме двух `.xex`, обнаружены 15 файлов `.dll` с настоящей сигнатурой XEX2
в `sp/<level>/`: blkbrn, chambois, credits, crssrds, falaise, forest,
fuelplnt, hostage, island, laison, mace2, mayenne, nightd, saint_lo, stbert.

Ресурсы по расширениям: 451 `.cod`, 30 `.wbk`, 20 `.wma`, 20 `.wmv`,
13 `.cfg`, один `.ttf`. Форматы COD/WBK и зависимости ещё не расшифрованы.
Первые 16 байт и размеры всех файлов записаны в `reports/game-analysis.json`.

## Ghidra и секции

Ghidra 12.1.3 + XEXLoaderWV 12.1.3, JDK 21, реальный headless import.
Архитектура `PowerPC:BE:64:A2ALT-32addr`, загрузчик зарегистрировал 10 896
функций из метаданных. База проекта локальна в `reports/ghidra/cod3`.

| Секция | Адрес | Размер |
|---|---|---|
| .rdata | 82001000 | 80BC0 |
| .pdata | 82082000 | 15480 |
| .text | 820A0000 | 4D8E8C |
| .data | 825A0000 | 64AAFC |
| .tls | 82BEB000 | 1F |
| .idata | 82BF0000 | 36A |
| .reloc | 82C01000 | 4C208 |

Есть семь дополнительных executable `.embsec_` в 82579000–8259B1B7;
SDK code range 820A0000–8259B1B8 не равен одной PE .text секции.

## Импорты и запуск

Заголовок содержит xboxkrnl.exe (282 address records) и xam.xex (86 records).
Не путать количество records с количеством функций. Runtime разрешил
146 xboxkrnl и 43 xam imports; generated header содержит 179 callable imports.
В их числе синхронизация, XEX loading, Vd GPU, файловые и XAudio функции.
Полный перечень имён доступен в ignored generated/default/cod3pc_funcs.h.

Реальный запуск зафиксировал `COD3_ORIGINAL_ENTRY_ENTER guest=0x82344D00`,
затем GPU callback 822F1918, render targets, shader pipelines, XAudio callback
822C4B48 и SDL audio endpoint 48000 Hz. Это подтверждает выполнение оригинального
кода и начало системной инициализации; отображение меню пока не подтверждено.

## Исправления анализа

Изначально две UnresolvedCall. Ghidra подтвердил:

- 822D0498: `4BFFFC80`, tail branch к 822D0118; размер 4, затем zero padding.
- 822D2140: `38630004` (r3 += 4), `4BFF98E4` (tail branch к 822CBA28); размер 8.

Уточнения в `cod3pc/configs/functions.toml` устранили обе ошибки строгой
валидации. Generated C++ не редактировался; пустых заглушек не добавлялось.
При следующем runtime выявлен indirect target 822C27F8. Ghidra подтвердил
валидный вход после bctr, операции над символом/таблицей. В конфигурацию добавлен
только entry; размер оставлен алгоритму control-flow SDK.

## Открытые источники и границы применимости

[ReXGlue v0.10.0](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.10.0)
закреплён по commit c94f5ebdcb3c9d1a460ca48e04f9758448f8d518.
README, Wiki, release, issues и история изучены; локальный CLI отличается
от примеров Wiki, поэтому команды подтверждаются установленным бинарником.

[CoD Xe](https://github.com/michaeloliverx/codxe) commit
421fe682c5ce069b2f8e03ddd1b6999c23644d07: NGL поддержка заявлена для MP TU3,
SP — No. `src/game/ngl/mp/symbols.h` содержит Cvar_Get 82514AF0,
Cvar_RegisterCommands 82517F80, PM_ClipVelocity 8244CFC8. Это только справка
для другого модуля/TU; не функции нашей кампании. Закрытый Xbox SDK не установлен.

[Xenia Canary](https://github.com/xenia-canary/xenia-canary) остаётся эталоном
для будущего сравнения запуска, файлов и GPU. README и releases изучены;
реальный эталонный запуск пока не проведён. XenonRecomp/XenosRecomp не нужны
для первой генерации: сохраняется ReXGlue и штатный Xenos backend.
