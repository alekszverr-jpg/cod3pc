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

Дополнительно Ghidra подтвердил 822CAB78 (6 инструкций, 24 байта) и
8216D720 (r3 += 4; tail branch 8216D600, 8 байт). После этого runtime
остановился на 822D2508 — такой же this-adjustment thunk.
`ExportCod3Thunks.java` нашёл ещё девять точных двухинструкционных thunks,
на которые ссылаются реальные слова в .rdata и которые переходят в уже
зарегистрированные функции. Проверяются оба PPC opcode, execute permission,
выравнивание и Ghidra disassembly. Результат в `configs/ghidra_thunks.toml`;
никакие функции не подменяются, регистровая поправка и tail branch сохраняются.

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
# Runtime diagnosis 2026-10-12: Saint-Lo startup

Saint-Lo native DLL registers 6071 functions. The first Debug launch reaches
module initialization then faults on guest 0x33B7358B. An optimized -O2 run
with `--cl "+spmap saint_lo"` reproduces a fault on guest 0x00005104, at
`__imp__sub_89132580`, generated line for guest instruction 0x89132800.
LLDB reads the local dump and confirms the caller chain
89097940 -> 8909DD28 -> engine 824A6150. Diagnostic hooks show the guest stack
has moved from 7016D5B0 to 7016F7F0 before the later initializer, with r30/r31
zero. This is consistent with nonlocal context restoration followed by an
ordinary C++ return; a context-changing path must unwind its host frames.

Ghidra independently confirms setjmp entry 82351CC0 and longjmp 8234EEC0
(FPR14-31/GPR13-31 and CR/LR/SP save/restore). These addresses are now supplied
to the SDK's codegen configuration; its own generated PCH already implements
the host jump-buffer map and setjmp/longjmp helpers. No external host code is
copied and no generated function is edited. A focused optimized native test
passed nested host-frame unwinding, independent guest keys, key reuse and the
standard zero-to-one longjmp result. Game-level verification remains required;
saved script coroutine continuations need additional support.

Host fibers now retain the script's native call chain across timed, condition
and event waits; three exact Saint-Lo assembly wrappers restore their saved PPC
packet before returning. Ghidra evidence is retained locally, including
`ghidra-event-suspend.log` and the generated yield-hook review. No generated
game function is edited. Optimized native continuation and FPU tests pass.

Runs `native-20261012-145033-379.log` and `native-20261012-150013-360.log`
progress beyond the earlier C0000028 event-wait failure, with many original
script starts and resumes. The latter stops on our duplicate-start invariant:
key AA32BA00, unchanged object AA2DD688, flags 00000800, two resumes.
This is not proof of gameplay. The original scheduler's release function
824A5D70 unlinks the script, calls 824A3040, then returns it to the pool via
820CE938. Ghidra confirms function range 824A5D70..824A5E0F and the release
call at 824A5DF8 (opcode 4BC28B41), in the pinned default.xex hash above.
A raw hook now erases the retained host continuation only after this original
release completes; duplicate-start and active-release checks remain fatal.
Strict codegen, optimized build and all five tests pass; runtime verification
of the lifetime correction in `native-20261012-150633-931.log` shows original
script releases and address reuse without the earlier duplicate-start error.
The next fatal call is indirect target 890C1950. Ghidra in saint_lo.dll confirms
890C194C branches backwards to 890C1900, 890C1950 is blr (4E800020), and
890C1954 is zero padding. A four-byte function entry is added in
configs/saint_lo_functions.toml, pinned to the Saint-Lo hash. This recovers an
original return-only function; no guest implementation is replaced.
