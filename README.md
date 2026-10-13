# Call of Duty 3 native Windows port research

Практический проект статической рекомпиляции Xbox 360 → Windows x64 на ReXGlue.
Цель первого игрового этапа — оригинальное меню, затем одиночная кампания.
Текущий подтверждённый результат и ограничения: [docs/STATUS.md](docs/STATUS.md).
План этапов и критерии готовности: [docs/ROADMAP.md](docs/ROADMAP.md).

Открывайте эту папку в VS Code. Полная IDE Visual Studio не требуется;
компилятор, MSVC и Windows SDK предоставляют отдельно установленные Build Tools.
Команды VS Code: Terminal → Run Task → `cod3: build`.

```powershell
./tools/Check-Environment.ps1
./tools/Build.ps1 -Stage Tests
./tools/Build.ps1 -Stage Analyze
./tools/Build.ps1 -Stage Codegen
./tools/Build.ps1 -Stage Build
```

Ресурсы собственной копии хранятся в `assets/game/` с исходной структурой.
Имеющийся GoD-контейнер `415607E1/00007000/2E97F145D2D59518C2F4`
извлечён локально. Игровые данные, generated C++, бинарники, логи и базы
Ghidra исключены из Git. Никакие игровые файлы не отправлялись на серверы.

См. [сборку](docs/BUILDING.md), [исследование](docs/RESEARCH.md),
[архитектуру](docs/ARCHITECTURE.md), [тестирование](docs/TESTING.md).
