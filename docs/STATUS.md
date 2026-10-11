# Состояние проекта

Обновлено 2026-10-12 (Asia/Yekaterinburg). Подтверждён M4 — Original Code
Execution. Меню и миссия не запущены, M5–M8 не достигнуты.

| Milestone | Доказательство / состояние |
|---|---|
| M0 Environment Ready | Build Tools, Windows SDK, LLVM 23, CMake, Ninja; официальный SDK собран из исходников и установлен; настоящий проект успешно собран |
| M1 Game Analysis | XEX headers, SHA-256, секции Ghidra, imports runtime, inventory и первичные блокеры подтверждены; исследования движка продолжаются |
| M2 Recompiled Code | Строгая генерация default.xex без force успешна после Ghidra-проверенных уточнений; косвенные runtime targets требуют дальнейшей регистрации |
| M3 Native Build | Windows x64 cod3pc.exe успешно собран clang++ 23.1.3, оригинальный generated C++ включён |
| M4 Original Execution | Реальный запуск: COD3_ORIGINAL_ENTRY_ENTER 82344D00, GPU pipelines и audio callback оригинального кода |
| M5 Main Menu | Не достигнут: запуск останавливается на недостающей функции; экран меню не подтверждён |
| M6–M8 | Не достигнуты |

Извлечены все 552 файла (6 190 037 625 байт). Полный inventory включает
17 XEX images: два EXE-модуля и 15 DLL уровней. Все материалы остались локально.
Данные, generated game code, бинарники и логи исключены из Git.

Работают: воспроизводимая init/scaffold, codegen, native build, VFS mounting,
вход в xstart, начало GPU и аудио инициализации. 3 parser tests прошли.
Играбельность, корректность картинки/звука, загрузка миссий, мышь, сохранения,
стабильность и производительность не проверены.

Последние успешные проверки: строгий codegen с 822C27F8, нативная сборка;
запуск прошёл прежнюю ошибку 822C27F8 и остановился на 822CAB78.
Windows Computer Use подтвердил реальный CRT abort dialog.

Ошибки и исправления:

- MSYS tools / Windows Python alias → явные рабочие native paths.
- Старые init flags Wiki → фактический manifest CLI v0.10.0.
- libmspack Git symlink placeholders → безопасная материализация целей.
- UnresolvedCall 822D0498 и 822D2140 → точные размеры 4/8 из Ghidra.
- Template GPU disabled → официальный xenos plugin и GPU_PLUGINS xenos.
- Entry trace symbol → использовать фактический xstart, не sub_82344D00.
- Runtime 822C27F8 → проверен в Ghidra и добавлен entry для SDK discovery.

Следующее действие: Ghidra-проверка 822CAB78, строгая регенерация, сборка,
повторный bounded runtime. Затем проверить оригинальное меню визуально,
сравнить Xenia и подготовить registry 15 XEX DLL для кампании.

Версии: ReXGlue 0.10.0 / c94f5ebdcb3c9d1a460ca48e04f9758448f8d518;
LLVM 23.1.3 для приложения, Clang 19.1.5 в первой исходной сборке SDK;
Build Tools 17.14.41, MSVC 14.44.35207, Windows SDK 10.0.26100.0;
CMake 4.4.4; Ninja 1.13.2; Git 2.55.0.windows.3; Python 3.12.14;
Ghidra/XEXLoaderWV 12.1.3; Temurin JDK 21.0.12.101.
