# Состояние проекта

2026-10-20: возобновлена диагностика огоньков, один PID 16764, профиль
nightd_glow_query_01. Пользователь выполнил hidden/direct GetData probe:
384 записи, один caller 0x82418264, готовые результаты после загрузки 1000.
Связь этого запроса с локальной лампой ещё не доказана. В нашем XEX проверен
кандидат чтения resolve-пикселя 0x82420F48; запрошено переключение горячего
readback_resolve=full в текущем окне, без сохранения config. Результат ожидается.
Подробности и ограничения: [GLOW_VISIBILITY.md](GLOW_VISIBILITY.md).

Следующая сессия: пользователь отложил проверку огоньков до завтра и сам
закроет игру. Два полных GPU-кадра сохранены. Read-only GetData trace собран,
16 tests/strict codegen/native build прошли. Профиль nightd_glow_query_01,
последний PID 21320, лог native-20261020-001341-880.log. Нужно открыть один
экземпляр через меню, Continue, затем rearm cod3_gpu_query_trace через F4
для вида за стеной и прямого вида лампы. Огоньки пока не исправлены.

2026-10-19: принята правка глубины материала Night Drop: пользователь
подтвердил исчезновение тени сквозь стену и сохранение обычных теней.
Парные снимки с переключателем on/off также подтверждают исчезновение
чёрных пятен кладки при сохранении рельефа. Стандартный Run-Native включает
правку; -OriginalShadowDepth оставлен для сравнения. Приёмка ограничена
идентифицированным материалом и проверенной сценой. Далее — огоньки.

2026-10-19: три полных GPU-кадра локализовали тень в материале стены, ucode
881F60CEB930E66D точно сопоставлен с захватом повторной трансляцией. Опытная
замена компонента глубины убирает силуэты сквозь низкую стену и простенок в
RenderDoc. Добавлен отдельный -ShadowDepthSource (по умолчанию выключен,
отдельный pipeline key). GPU/16 tests/strict codegen/native build прошли.
Подготовлена копия nightd_shadow_depth_01; нужны приёмка в игре и проверка,
что обычные тени на земле сохранились. Огоньки этим экспериментом не исправлены.

2026-10-19: полный guest GPU-кадр проблемной сцены успешно снят (frame
11029→11030, success=1, 2427 draws). В материале стены найден кандидат
shadow depth atlas с силуэтами персонажей; проверка сравнения глубины ещё
идёт. Отдельное подозрение на новый дефект кладки пользователь отозвал.
Тени и огоньки сквозь стены пока не исправлены; текущая игра не прерывалась.

2026-10-19: -OcclusionSnapshots даёт настоящие GPU результаты и сохраняет
queries доступными, но пользователь подтвердил: просвечивание осталось.
Режим не принят как исправление и выключен обычным launcher явно. Подготовлен
одноразовый RenderDoc capture по границам guest GPU-кадра; build/tests/codegen
прошли, стартовый захват success=1. Открыт один PID 27596/nightd_guest_frame_01
для полного захвата проблемной сцены. Тени и огоньки пока не исправлены.

2026-10-19: диагностика доказала отключение host occlusion после пары ZPD
по разным адресам, затем постоянный samples=1000. Подготовлен opt-in режим
настоящих накопительных счётчиков -OcclusionSnapshots с fence-защищёнными
сегментами. GPU/16 tests/strict/native build прошли; открыт один PID 62736,
копия nightd_occlusion_snapshots_01. Нужна приёмка hidden/visible огоньков;
исправление просвечивающей тени пока не установлено.

2026-10-19: начат разбор shadow/glow occlusion по десяти GPU-захватам.
Glow draws локализованы: глубина quad z=w=1, depth test включён; в трёх
проверенных captures нет host Query chunks. Причина fallback ещё не доказана.
Подготовлена opt-in диагностика -OcclusionTrace без изменения результатов
рендеринга; GPU/16 tests/strict/native build прошли. Открыт один PID 62028
с копией nightd_occlusion_trace_01. Нужен Continue и повтор сцены; это пока
диагностическая сборка, исправление просвечивания не заявлено.

2026-10-19: стандартный запуск → Night Drop → меню → Continue принят.
Пользователь подтвердил прежнее окно и быструю загрузку; лог
native-20261019-154208-583.log: generation=2, PID 23612/HWND 0x160cb2 прежние,
clean Runtime/script drain, DLLs 1 → 0, повторный Night Drop entry result=1
в 15:46:07, FATAL нет. Приёмка нового launcher default завершена; скорость
не измерялась отдельным benchmark. Текущая игра оставлена открытой.

2026-10-19: первый обычный запуск нового preset принят пользователем:
Night Drop загружена корректно. native-20261019-154208-583.log подтверждает
generation=1, PID 23612/HWND 0x160cb2, original entry и Night Drop entry
result=1 в 15:43:16; FATAL нет. На момент проверки generation=2 ещё нет,
поэтому именно повторный menu/Continue нового launcher пока не заявляется.

2026-10-19: стандартный Run-Native.ps1 теперь включает принятые same-process
переходы и все барьеры автоматически. -ProcessRelaunch оставлен для старого
диагностического пути; stock/experimental GPU требуют его явно. CLI проверки,
16 tests, strict codegen и native CMake build прошли. Прямой exe сохраняет
отдельные SDK CVar defaults. Текущая игра не прерывалась; проверка нового
launcher preset в игре — при следующем обычном запуске.

2026-10-19: пользователь подтвердил сохранение оконного режима после
меню → Continue. Вместе с ранее принятыми настройками, двумя переходами
миссий и повторными menu/Continue это закрывает запланированную ручную
приёмку same-process пути. Включение по умолчанию — следующий отдельный
шаг; флаги пока не менялись. Полная кампания и графические дефекты не приняты.

2026-10-19: пользователь отдельно подтвердил сохранение настроек после
меню → Continue. Этот критерий принят. Переключение оконного/полноэкранного
режима и его сохранение пока отдельно не проверены; SDK fullscreen=true
по умолчанию поддерживает также false.

2026-10-19: Night Drop → меню → Continue принят пользователем. Лог
native-20261019-143620-467.log подтверждает generation=2 в том же PID 8180/
HWND 0x190c6a, чистый teardown и повторный original entry result=1 в 15:21:49.
FATAL в проверенных файлах нет. Отдельного подтверждения сохранения
полноэкранного режима и настроек пока нет; этот критерий остаётся открытым.

2026-10-19: пользователь прошёл прежнее место вылета Night Drop без аварии.
Правка границы 0x89174700 принята на проблемном участке; текущий лог
native-20261019-143620-467.log не содержит FATAL, игра продолжает работу.
Полное прохождение Night Drop ещё не подтверждено. Следующая отдельная
приёмка same-process режима — fullscreen/settings при menu → Continue.

2026-10-19: Night Drop target 0x89174700 подтверждён Ghidra как настоящая
четырёхбайтная blr-функция после bctr, пропущенная анализатором. Добавлена
граница в nightd_functions.toml; strict codegen/16 tests/native build прошли.
Открыт один nightd_boundary_fix_01 (PID 8180) с копией completed-Island save
для Continue и повторения вылета. Игровая приёмка исправления ещё ожидается.

2026-10-19: обновлён перевод из прежней пользовательской папки Xbox 360.
Полное сравнение SHA-256 обнаружило только sp/frontend.cod; файл установлен
при закрытой игре, предыдущий сохранён, хеш после копирования проверен.
Исполняемые файлы не менялись; пересборка не нужна. Сохранения не затронуты,
визуальная приёмка новой ревизии ещё впереди. См. LOCALIZATION_098.md.

2026-10-19: Island → Night Drop принят в том же процессе и окне. Лог
native-20261019-100318-033.log: generation 1/2, PID 58744, HWND 0x1a06dc;
в 10:08:56–57 все drain complete, pending threads/refs=0, contexts/fibers=0,
globals cleared, campaign DLLs 1 → 0. Night Drop entry result=1 и load status=0
в 10:09:26; пользователь подтвердил загрузку и нормальную работу. island_final
теперь проверен как точка до completion. Остались fullscreen/settings перед
решением о default; вылет Night Drop по 0x89174700 ещё не исправлен.

2026-10-19: новая точка пользователя перед финалом Island сохранена в
island_final после разрешённого штатного закрытия, SHA-256 проверены.
Открыта её отдельная копия island_final_inprocess_01 (PID 58744) для приёмки
Continue и Island → Night Drop в том же процессе/окне. Запись ISLAND/i_event_6
сама по себе не доказывает положение до completion; ждём игровую проверку.

2026-10-19: пользователь продолжает Island с отдельной копией i_event_2
(island_event2_inprocess_01); ожидается checkpoint до завершения миссии.
Вчерашний вылет Night Drop найден: native-20261018-224203-062.log,
22:48:36.891, invalid/unregistered guest function 0x89174700. Это ещё
неисправленный блокер прохождения; причина требует Ghidra-проверки.
Скриншоты тени и огоньков поверх стен зафиксированы в GPU_RENDERING.md.

2026-10-19: готового финального Island checkpoint до completion не найдено.
Копия ISLAND/i_event_6 уже ведёт Continue в Night Drop (подтверждено пользователем).
Остальные снимки — i_event_1/2. Legacy loadgame требует отдельный SVG на диске;
тест в отдельном профиле вернул NO_SUCH_FILE, файлы Xbox save не редактировались.
Непрошедший launcher-эксперимент удалён. Проверка Island → Night Drop остаётся
отдельной от прямой загрузки Night Drop. См. CHECKPOINT_TESTING.md.

2026-10-18: повторный цикл меню → Continue принят в том же окне: generation=3
с прежними PID 51848/HWND 0x1609da в 22:35:11, повторный teardown чистый,
Island entry/loader успешны в 22:35:41. Пользователь подтвердил меню и Continue.
Таким образом, в одном процессе прошли уже 3 поколения Runtime. Следующая
приёмка — Island → Night Drop и fullscreen/settings; default пока не меняется.
Лог native-20261018-222816-941.log, ранние записи ротированы в .1.log.

2026-10-18: первый Saint-Lo → Island В ОДНОМ ПРОЦЕССЕ И ОКНЕ принят.
Лог native-20261018-222816-941.log: generation 1/2 с PID 51848 и HWND 0x1609da,
чистые native/script/input drains, globals cleared, DLLs 1 → 0, восстановлены
664 байта; Island entry result=1/LOAD_RESULT=0 в 22:32:11. Пользователь подтвердил
загрузку, управление и звук. Child process не запускался. Режим пока opt-in;
следующая приёмка — меню → Continue и повторный restart в том же экземпляре,
затем fullscreen/settings. Игра оставлена открытой. См. INPROCESS_RESTART.md.

2026-10-18: первый same-process тест достиг generation=2 при прежних PID/HWND
и повторного original entry, но завис на SDL input. Дамп выявил watch со старым
userdata: SDL-драйвер не очищался при сохранённом окне. Добавлен UI detach с
удалением watch/listener и drain pending pump; защищённый native canary до
правки падал 0xC0000005, после проходит. 16 tests, strict/full build прошли.
Открыт один saint_lo_inprocess_input_01 (PID 51848), лог
native-20261018-222816-941.log; игровая приёмка ещё нужна. См. INPUT_RETIREMENT.md.

2026-10-18: чистый teardown принят: Saint-Lo сняла регистрацию 6073 функций,
DLLs 1 → 0, globals cleared; прежних ошибок heap/module sweep нет в
native-20261018-214705-386.log, Island загружена. Подготовлен opt-in
TitleInProcessRestart: сохранение HWND/PID, новый Runtime/presenter/input и
оригинальные 664 байта launch data без child process. 16 tests, strict codegen
и изменённые app objects прошли. После разрешённого штатного закрытия Island
full build установлен; открыт один saint_lo_inprocess_01 (PID 38720), лог
native-20261018-220651-003.log. Первая same-process приёмка ожидается.
См. INPROCESS_RESTART.md.

2026-10-18: старый Runtime действительно уничтожился при живом окне,
globals_cleared=true, campaign_dlls 1 → 0; Island загрузилась. Но в teardown
найдены ошибки module sweep и heap Release. Реальный XEX probe доказал:
загрузка DLL сбрасывала учёт памяти EXE, а resolved device path не находился
в реестре game-relative библиотек. Исправлены shared-heap Reset и VFS lookup;
последовательные загрузки 3 DLL, 16 tests и strict codegen прошли. Чистая
игровая приёмка исправлений ещё нужна. Native build установлен с совпадающими
DLL SHA; открыт один saint_lo_xex_ownership_01 (PID 2736), лог
native-20261018-214705-386.log. См. XEX_OWNERSHIP.md.

2026-10-18: script TLS принят: contexts=0 fibers=0 complete=true,
reaped=19 pending=0 после всех native drains в native-20261018-153758-546.log;
Island загружена. Подготовлен opt-in TitleRuntimeTeardown: watcher сначала
отпускает main XThread, затем UI освобождает overlays и старый Runtime при
сохранённом окне. 16 tests, strict codegen/full build прошли. Открыт один
saint_lo_runtime_teardown_01 (PID 39580); результат ещё нужен. Переход пока
межпроцессный. См. TITLE_RUNTIME_TEARDOWN.md.

2026-10-18: ThreadStartData правка принята: Island загружена, четыре drain
успешны, reaped=19 pending=0 в native-20261018-094956-224.log. Следующий
этап — script TLS: добавлены фактические счётчики контекстов/fibers и лог их
очистки после native exit. Probe с настоящим приостановленным Cod3FiberTask
подтвердил 1 → 0; 16 tests, strict codegen и изменённые app objects прошли.
Full native build/install также прошёл. Открыт один saint_lo_script_tls_01
(PID 4968), лог native-20261018-153758-546.log подтверждает original entry.
Игровая приёмка новой диагностики ещё нужна; same-window reset не включён.

2026-10-18: переход saint_lo_exit_refs_01 принят: Island загрузилась, все
четыре drain-барьера завершены, COD3_TITLE_EXIT_REFS reaped=19 pending=0
в native-20261018-093413-667.log. Следующий probe воспроизвёл отдельную утечку
ThreadStartData; Windows TLS-владение прошло normal/Exit/secondary-fiber
сценарии и три цикла Runtime. Все 16 tests, strict codegen и native build
прошли; дополнительная SDK-правка установлена с совпадающими DLL SHA.
Открыт один saint_lo_start_data_01 (PID 39792), результат перехода ожидается.
Следом — владение script TLS/fibers, модулей и сохранение окна.
Переход всё ещё межпроцессный. См. THREAD_EXIT_OWNERSHIP.md.

2026-10-18: self-reference завершившегося XThread перенесена в явную очередь
освобождения после native exit; thread-fiber освобождается без изменения
fiber контекста reaper. Изолированные 3 цикла Runtime и blocked-owner проверка
прошли, всего 16 tests. Strict/native build установлен с совпадающими DLL SHA.
Открыт один saint_lo_exit_refs_01 (PID 29220) для проверки настоящего перехода;
результат ожидается. Полный same-window reset ещё не включён.

2026-10-18: Night Drop принята пользователем по загрузке и управлению. Лог
native-20261017-153108-502.log: entry 89201B08 reason=1/result=1, LOAD_RESULT=0.
Полное прохождение третьей миссии ещё не подтверждено. Найдена отдельная SDK
утечка self-reference XThread после native exit; исправление проверяется
изолированным lifecycle-тестом, не подменяющим игру. См. THREAD_EXIT_OWNERSHIP.md.

2026-10-17: пользователь подтвердил пропуск брифинга и полное прохождение
Island. На загрузке третьей миссии отсутствовал статически собранный nightd.dll
(лог Execute(89201B08) missing, затем FATAL 890689D0). Guest/audio/GPU/dispatch
барьеры Island при этом все прошли. Подключён настоящий nightd: Ghidra entry
и 3 yield wrappers подтверждены, strict codegen/native build и 15 tests прошли.
Правильное debug-имя nightdrop проверено по оригинальному XEX. Runtime-приёмка
третьей миссии ещё нужна; одно окно/процесс пока не реализованы. См. NIGHTDROP.md.

2026-10-17: пропуск загрузочного брифинга по Пробелу/Enter реализован после
оригинального ожидания ресурсов, с исходной процедурой закрытия видео/звука.
Правка границы Island 891999E0 установлена. Strict/native build и 15 tests
прошли; DLL Runtime в приложении и SDK совпали. Открыт один island_movie_skip_01
(PID 33136) с копией Island checkpoint; игровой результат ещё ожидается.
См. MOVIE_SKIP.md. Бесшовный reset остаётся незавершённым.

2026-10-17: Kernel Dispatch принят: native_exited=true, retired=0, complete=true;
Island загрузилась. Граница оригинальной функции Island 891999E0 подтверждена
Ghidra, строгий codegen и компиляция изменённых объектов прошли; установка
и повтор входа в здание ещё нужны. По просьбе пользователя исследуется пропуск
роликов после готовности уровня. Бесшовный reset ещё не включён.

2026-10-17: GPU-барьер подтверждён: command/vsync native exit, queue_idle=true,
callbacks_idle=true и отсоединение presenter; Island загрузилась. Добавлен
opt-in Kernel Dispatch drain, SDK/15 tests прошли, full build и runtime-проверка
ещё впереди. В Island при входе в здание отдельный FATAL 0x891999E0; причина
не установлена. Бесшовный reset ещё не включён. См. SEAMLESS_TRANSITIONS.md.

2026-10-17: после разрешённого закрытия Island завершены strict/native build
и установка GPU drain. SHA-256 собранной/установленной GPU DLL совпали.
Открыт один saint_lo_gpu_drain_01 (PID 58864) с копией финального checkpoint,
DoF 35 и всеми drain flags; исходный entry подтверждён. Runtime-результат GPU
пока ожидается. Дальше — Kernel Dispatch/владение контекстами и очистка Runtime;
переход в одном процессе ещё не включён. См. SEAMLESS_TRANSITIONS.md.

2026-10-17: audio/XMA barrier подтверждён: audio_exited=true,
xma_exited=true, callbacks_idle=true, закрыт 1 SDL driver; пользователь подтвердил
Island без зависания и проблем звука. Подготовлен opt-in GPU/presentation drain,
plugin/app objects скомпилированы, 15 tests прошли. Замена загруженной GPU DLL
пока заблокирована открытой Island; ответ на запрос закрытия ожидается.
Полная линковка/установка и runtime-проверка GPU ещё нужны. Runtime и окно
при переходе пока не пересоздаются в одном процессе. См. SEAMLESS_TRANSITIONS.md.

2026-10-17: реализован opt-in TitleAudioDrain после успешного guest drain:
закрывает admission аудиовызовов, кооперативно останавливает audio/XMA,
проверяет native exit и затем закрывает SDL drivers. При timeout сохраняет
ресурсы и возвращает failure, без force-kill. Build/15 tests прошли;
открыт saint_lo_audio_drain_01, runtime-проверка ожидается. GPU/UI/dispatch
и in-process reset ещё не завершены. См. SEAMLESS_TRANSITIONS.md.

2026-10-17: SDK TitleThreadDrain подтверждён в runtime:
native-20261017-094421-086.log — sealed=true, creation_complete=true,
tracked=40, pending=0; 13 captured также завершены. Successor восстановил
664 байта, Island DllMain вернул 1, LOAD_RESULT=0; пользователь подтвердил
загрузку. Следующий этап — отдельный барьер audio/XMA, затем GPU/callbacks.
Одно окно/процесс пока не реализованы. См. SEAMLESS_TRANSITIONS.md.

2026-10-17: предыдущий bounded-wait тест успешен: пользователь подтвердил
Island, лог native-20261016-203735-755.log показывает 13 captured / 0 pending
до handoff. Добавлен следующий opt-in TitleThreadDrain: SDK запрещает новые
guest Create/Restore после seal, удерживает объекты потоков и проверяет
фактический native exit после завершения создания. Strict/native build и
15 tests прошли. Открыт saint_lo_thread_drain_01 для runtime-проверки.
Это ещё не барьер audio/GPU и не бесшовный reset. См. SEAMLESS_TRANSITIONS.md.

2026-10-16: дамп старого PID 5956 сопоставлен со всеми тремя pending workers:
они ждут NtWaitForSingleObjectEx в idle-пути 8212A520. Реальные SDK wait checks
стоят до/после native wait. Ghidra подтвердил точный caller/timeout; расширен
opt-in CooperativeWorkerExit: настоящий wait по 50 ms и self-exit между
вызовами. Strict build/codegen и 14 tests прошли. Открыт отдельный checkpoint
saint_lo_bounded_wait_01, результат перехода ожидается. Безопасный in-process
reset пока не включён. См. SEAMLESS_TRANSITIONS.md.

2026-10-16: добавлен opt-in TransitionDump: автоматический out-of-process
захват стеков именно старого PID после обнаружения pending workers и до
межпроцессного handoff. Build/14 tests и проверка dump helper прошли.
Открыт saint_lo_thread_dump_02 для перехода с финального checkpoint.
При аудите обнаружены отдельные барьеры audio/XMA (force termination/abandon
на timeout), которые также требуется устранить перед безопасным reset.
Бесшовный переход пока не включён. См. SEAMLESS_TRANSITIONS.md.

2026-10-16: пользователь подтвердил работу перевода 098 в нативном порте.
На присланном кадре сцены Saint-Lo русские субтитры отображаются читаемо.
Полная проверка вёрстки всех миссий и меню этим не подтверждается.

2026-10-16: ресурсы заменены пользовательской русской сборкой 098, ревизия 05.
Проверены SHA-256 всех 552 файлов при копировании; предыдущая версия сохранена
в reports/localization-098/previous-game, userdata не изменён. Независимое
сравнение секций Ghidra подтверждает прежние границы функций: изменены фильтр
символов целей и стартовая строка загрузки. Strict codegen/native build и все
14 tests прошли. Открыт один обычный запуск с DoF 35%; визуальная приёмка
перевода ещё требуется. См. LOCALIZATION_098.md. Блокер бесшовного перехода
между миссиями остаётся открытым.

2026-10-15: эксперимент CooperativeWorkerExit не устранил блокер. Лог
native-20261015-173618-681.log: WORKER_EXIT_ARMED, 13 captured, 3 pending
(guest 8/9/10, start=8212A780), ни одного WORKER_EXIT_IDLE. Пользователь
подтвердил загрузку через новый процесс. Бесшовный режим не реализован;
эксперимент остаётся выключенным по умолчанию. Следующий шаг — стеки именно
старых pending threads и порядок завершения внутри SDK, не повтор того же теста.

2026-10-15: runtime probe выявил три живых потока из 13 после завершения
Saint-Lo (ожидание 1000 ms). Добавлен выключенный по умолчанию эксперимент
CooperativeWorkerExit: проверенные worker idle boundaries и штатный XThread::Exit
только при оригинальном same-title restart. Strict codegen/build и 14 tests
прошли; открыт saint_lo_worker_exit_01 для сравнения. Это ещё не бесшовный режим.
Стеки текущей Island сняты приватным dump и разобраны offline. См.
SEAMLESS_TRANSITIONS.md.

2026-10-15: добавлен первый этап бесшовного lifecycle — наблюдение реального
завершения native guest threads через дублированные Windows handles перед
LaunchTitle и после выхода main guest thread. Strict codegen/build и 14 tests
прошли. Это пока диагностика, не безопасный in-process reset: поздние потоки
и GPU/audio workers требуют отдельного барьера. См. SEAMLESS_TRANSITIONS.md.

2026-10-15: пользователь изменил критерий перехода: одно окно и процесс,
без native relaunch. Это ещё не реализовано. Исследованы ограничения pinned
SDK shutdown: возможные straggler threads и _Exit при закрытии окна.
Следующий приоритет перенесён с дальнейшего прохождения Island на безопасный
in-process lifecycle. Проверенный межпроцессный переход сохранён для сравнения;
текущая игра не закрывалась и не перезапускалась. См. SEAMLESS_TRANSITIONS.md.

2026-10-15: подтверждён настоящий переход Saint-Lo → Island. Пользователь
завершил Saint-Lo с восстановленного checkpoint saint_lo_final и подтвердил
загрузку/управление в Island после автоматического native relaunch.
native-20261015-165906-292.log: SetLaunchData 664 байта, LaunchTitle,
RELAUNCH_QUEUED; successor -relaunch.log: RESTORED 664, original GetLaunchData
успешен, Island entry 89226748 reason=1/result=1, LOAD_RESULT=0, без FATAL.
Старый PID 20020 завершён, работает только новый PID 34696. Отдельный профиль
и DoF 35 сохранены; основной прогресс Island и снимок не затронуты.
Переход проверен один раз; полное прохождение Island и остальные SP DLL
остаются открытыми. См. TITLE_RELAUNCH.md.

2026-10-15: сохранён снимок saint_lo_final из профиля saint_lo_end, файлы
проверены по SHA-256 и восстановлены в saint_lo_repeat_01. Лог предыдущего
запуска впервые подтвердил SetLaunchData=664, LaunchTitle default.xex и SDK
device path. Исправлена проверка пути в relaunch policy; strict codegen/build
и 13 tests прошли. Открыт один обычный запуск копии checkpoint; точное место
Continue и переход Saint-Lo→Island ещё требуют подтверждения пользователя.

2026-10-15: реализован native title relaunch с передачей 664 байт через
анонимный Windows pipe и ожиданием завершения предыдущего процесса до создания
окна/Runtime. Обычный выход не перезапускается. Strict codegen/build и 13 tests
прошли, включая реальную передачу синтетических байт между процессами.
Игровой переход ещё не подтверждён. См. TITLE_RELAUNCH.md.
Добавлены Checkpoint.ps1 и DebugProfile для точных снимков и повторов checkpoint
в отдельном профиле. Текущий Island скопирован/восстановлен с проверкой SHA-256;
снимка конца Saint-Lo ещё нет. См. CHECKPOINT_TESTING.md.

2026-10-15: исследован original title reboot для перехода кампании.
Ghidra независимо подтвердила две записи 664 байт launch data и startup read.
Добавлены read-only hooks размеров/статусов и лог loader state при завершении
главного guest thread. Strict codegen/build и 12 tests прошли. Поведение
перехода не изменено; нужен реальный повтор завершения Saint-Lo.

2026-10-14: добавлен отдельный debug selector: ListLevels / DebugMission /
DryRun, собственный userdata на миссию, защита от дубликата процесса.
Прямой debug-запуск Island через devmap подтвердил исходную загрузку DLL и
видимый уровень с оружием/HUD. 12 tests, strict codegen/build прошли.
Основной save не редактируется. См. LEVEL_DEBUG.md. В roadmap добавлены
PC-возможности по исследованию Project 1944; чужой код не перенесён.

2026-10-14: пользователь подтвердил загрузку Island и управление персонажем.
native-20261014-151651-140.log: исходный запрос island.dll, native Island entry
89226748 с reason=1 и result=1, LOAD_RESULT=0. Это подтверждает вторую миссию
в gameplay; полное прохождение Island и переход из Saint-Lo ещё проверяются.

2026-10-14: подключена native Island DLL (вторая объявленная SP DLL из 15).
Её hash, entrypoint и три script wait hooks проверены по оригинальному модулю.
Исправлена offline-генерация модулей с общим image base: явная overlay_group
и снимок кода каждой DLL до следующей загрузки. Strict codegen и native build
прошли; побайтное сравнение отдельных и общей генераций совпало для обеих
миссий, неверная группа отвергается. Восемь tests прошли. Ручная загрузка
Island и переход из Saint-Lo ещё не подтверждены. См. CAMPAIGN_READINESS.md.

2026-10-14: добавлена пользовательская настройка силы DoF при запуске:
`Run-Native.ps1 -DofStrength 0..100`, исходный режим 100 по умолчанию.
Она меняет только долю размытого изображения в установленном финальном
DoF shader; сила включена в ключ shader/pipeline cache. Strict codegen,
native/GPU build и восемь tests прошли. Открыт один обычный запуск с 35%:
native-20261014-095321-726.log подтверждает оригинальный entrypoint.
Пользователь подтвердил: стало чётче, фон слегка размыт. Дамп активного
шейдера содержит множитель 0.35; применение настройки подтверждено.
См. GPU_RENDERING.md
и BUILDING.md. Это дополнительная настройка, не доказанное исправление
расхождения с настоящей Xbox 360.

2026-10-13: пользователь сообщил полное прохождение первой миссии и закрытие
игры после неё. native-20261013-130703-196.log заканчивается Execution complete
в 13:23:28.224, без FATAL; запрос следующей DLL этим логом не установлен.
Добавлены Ghidra-проверенные COD3_TRANSITION_* hooks загрузки/выгрузки DLL,
LaunchTitle и двух точек TerminateTitle. Strict codegen, build и восемь tests
прошли. Для уточнения перехода открыт один обычный запуск с RenderDoc,
logs/native-transition-dof.log. Переход второй миссии ещё не подтверждён.
Новые проблемы: слишком сильный DoF при прицеливании (три пользовательских
кадра) и тихий/неверно расположенный звук. См. GPU_RENDERING.md и AUDIO.md.

Сохранения 2026-10-13: первоначально меню предлагало только новую игру,
пользователь не видел выбора storage или предупреждения; userdata содержал
только cache. После диагностического запуска и штатного «Выйти» появился
оригинальный COD3_SaveSPV1.0 (6928 байт) и header (328 байт). Лог завершился
Execution complete без FATAL. После нового запуска пользователь подтвердил
появление «Продолжить» и корректное восстановление контрольной точки.
Запись, обнаружение и загрузка этого сохранения Saint-Lo подтверждены.
Добавлены проверенные Ghidra
диагностические COD3_SAVE_* hooks без изменения игровых результатов.
Восемь tests, strict codegen и native build прошли. Следующая проверка —
продолжение Saint-Lo, повтор боя и завершение миссии.
См. [SAVES.md](SAVES.md); остальные checkpoint и переходы миссий ещё проверяются.

План следующих этапов: [ROADMAP.md](ROADMAP.md). Последняя проверка 2026-10-13:
первый проход участка с самолётом успешен, но повтор на том же участке завершился
вылетом. native-20261013-102856-653.log, 10:49:28.910: FATAL — invalid or
unregistered function 0x891A7A30 в Saint-Lo. Ghidra подтвердил оригинальную
одну инструкцию blr с padding после неё; граница 4 байта добавлена в
saint_lo_functions.toml. Стабильность участка и полный уровень не подтверждены.
Strict codegen, Windows RelWithDebInfo build и восемь tests прошли;
generated DLL registration содержит 0x891A7A30. Логи:
battle-repeat-tests-console.log, battle-repeat-build-console.log.
Новый ручной запуск — через меню для продолжения с checkpoint.
Следующие ручные запуски — одно окно через обычное меню, без -FirstMission,
чтобы пользователь мог продолжать с контрольной точки.

Устранённый сбой: пользователь прошёл поездку на грузовике и дошёл до боя.
Во время пролёта
самолёта над полем боя игра вылетела; native-20261013-101313-546.log,
10:25:36.202: FATAL — invalid or unregistered function `0x82498BD0`.
Ghidra подтвердил отдельную tail-call обёртку из четырёх инструкций (16 байт)
с переходом на 0x82452308. Добавлена граница в functions.toml; восемь тестов,
strict codegen и native build прошли (plane-target-tests-console.log,
plane-target-build-console.log). Обе функции присутствуют в generated
registration. Повтор проблемного участка без вылета подтверждён пользователем.

Предыдущий сбой после грузовика:
`native-20261013-095659-062.log`, 10:08:20.823: FATAL — invalid or unregistered
function `0x823C1F68`. Этот участок теперь пройден по сообщению пользователя;
полное прохождение миссии пока не подтверждено.

Функция проверена в Ghidra: 13 инструкций, размер 52 байта, завершающий
косвенный tail-call. Добавлена в functions.toml; восемь тестов, строгая
генерация и Windows RelWithDebInfo build прошли. Новая регистрация проверена
в generated function table. Логи: roadmap-truck-tests-console.log и
roadmap-truck-build-console.log. Повтор поездки и входа в бой подтверждён
пользователем; обнаружен следующий target, описанный выше.

Обновлено 2026-10-13 (Asia/Yekaterinburg). Подтверждён M4 — Original Code
Execution. Пользователь подтвердил меню и начало загрузки миссии в ручном
запуске. Ускорение видео после перехода на -O2 подтверждено пользователем.
Saint-Lo DLL подключена (6072 native функции); первый уровень загружен.
Управление персонажем подтверждено пользователем в том же запуске.
Прохождение и остальные уровни пока не подтверждены.

| Milestone | Доказательство / состояние |
|---|---|
| M0 Environment Ready | Build Tools, Windows SDK, LLVM 23, CMake, Ninja; официальный SDK собран из исходников и установлен; настоящий проект успешно собран |
| M1 Game Analysis | XEX headers, SHA-256, секции Ghidra, imports runtime, inventory и первичные блокеры подтверждены; исследования движка продолжаются |
| M2 Recompiled Code | Строгая генерация default.xex и saint_lo.dll без force успешна; конфигурация включает проверенные Ghidra coroutine hooks |
| M3 Native Build | Windows x64 cod3pc.exe и cod3pc_saint_lo.dll собраны Clang 23.1.3, RelWithDebInfo (-O2) |
| M4 Original Execution | Реальный запуск: COD3_ORIGINAL_ENTRY_ENTER 82344D00, GPU pipelines и audio callback оригинального кода |
| M5 Main Menu | Подтверждено пользователем в ручном запуске; автоматическая визуальная проверка пока не повторена |
| M6 In-Game | 2026-10-12, 15:13: визуально подтверждён первый игровой кадр Saint-Lo в нашем cod3pc: лагерь, солдаты, оружие, компас; пользователь подтвердил управление; продолжение после грузовика требует исправления и проверки |
| M7–M8 | Базовое управление персонажем подтверждено пользователем; прохождение, сохранения и длительная стабильность не подтверждены |

Извлечены все 552 файла (6 190 037 625 байт). Полный inventory включает
17 XEX images: два EXE-модуля и 15 DLL уровней. Все материалы остались локально.
Данные, generated game code, бинарники и логи исключены из Git.

Работают: воспроизводимая init/scaffold, codegen, native build, VFS mounting,
вход в xstart, GPU/audio init, загрузка DLL Saint-Lo. Прошли 6 parser/coverage tests
и 2 native tests: nonlocal jumps, сохранение host frames/FPU между fiber паузами.
Играбельность, полная корректность картинки/звука, остальные миссии, мышь, сохранения,
стабильность и производительность не проверены.

Первоначальные успешные проверки: строгий codegen, нативная сборка, запуск с
Ghidra-проверенными 822C27F8, 822CAB78, 8216D720 и девятью переходами из .rdata.
Лог native-20261012-001550-968.log подтверждает вход 82344D00, GPU pipelines,
audio callback и регистрацию контроллера. В этом логе нет FATAL; визуальная
проверка Windows Computer Use показала чёрный экран. Позднее пользователь
подтвердил меню; эта ранняя проверка не является текущим блокером меню.

Ошибки и исправления:

- MSYS tools / Windows Python alias → явные рабочие native paths.
- Старые init flags Wiki → фактический manifest CLI v0.10.0.
- libmspack Git symlink placeholders → безопасная материализация целей.
- UnresolvedCall 822D0498 и 822D2140 → точные размеры 4/8 из Ghidra.
- Template GPU disabled → официальный xenos plugin и GPU_PLUGINS xenos.
- Entry trace symbol → использовать фактический xstart, не sub_82344D00.
- Runtime 822C27F8 → проверен в Ghidra и добавлен entry для SDK discovery.
- Runtime 822CAB78 и 8216D720 → инструкции и размеры проверены в Ghidra.
- Runtime 822D2508 → автоматический поиск точных this-adjustment thunks по
  ссылкам .rdata; девять проверенных переходов добавлены в отдельный TOML.

Текущая работа: проверить обучение и первый бой Saint-Lo, затем переход между
миссиями на нашей основе ReXGlue. Загрузка первого уровня уже подтверждена.
Реализованы host fibers для сохранения живых цепочек вызовов сценария,
перехвачены четыре механизма паузы (frame, timed, condition, event) и три
точные обёртки сохранения PPC контекста в DLL. Все изменения подключены
codegen-конфигурацией и отдельными authored sources; generated C++ не редактируется.
Отсутствие FATAL не доказывает загрузку уровня; нужен реальный игровой кадр.

Запуск 15:06 остановился на indirect target 890C1950 после устранения
ошибки lifetime сценариев. Ghidra подтвердил оригинальную четырёхбайтную
функцию, она включена в codegen. Новый запуск native-20261012-151011-613.log
прошёл эту точку, продолжает исходные сценарии без FATAL (проверено 15:12).
Строгий codegen, оптимизированная сборка и пять тестов прошли. Визуальная
проверка в 15:13 подтвердила загруженный первый уровень с оружием и компасом.
Реальный процесс cod3pc.exe продолжает сценарии. Пользователь подтвердил:
«Уровень появился, персонаж управляется». Это не подтверждает полное прохождение.

Обновление после ручного запуска: `cod3pc/out/build/win-amd64-debug/logs/cod3pc_006.log`
в 00:30:28 содержит `Execute(892347A8): function not in function table` и
FATAL на `0x890985E0`. Наш inventory устанавливает 0x892347A8 как entrypoint
`sp/saint_lo/saint_lo.dll`, image base 0x89000000. DLL присутствует в данных,
но на тот момент её код ещё не был сгенерирован и связан с runtime. Этот блокер
устранён отдельной native DLL и module registry.
Черный экран раннего smoke test не означал невозможность запуска меню.
Сравнение альтернативной основы: `docs/COD3RECOMP_REVIEW.md`.

Проверка травы 2026-10-12: синхронный memexport, полный GPU readback и
поправка актуальности vertex buffers не устранили растянутые полосы. GPU
captures локализовали расхождение в расчёте позиции шейдера травы. Отдельный
плагин xenos-grass с правкой одного проверенного шейдера убрал длинные полосы
и сохранил динамическую траву в native запуске; это подтвердил пользователь.
Форма травинок остаётся неверной (короткие наклонённые ленты). Эксперимент
не включён по умолчанию, окончательный рендер не подтверждён. Следующий этап —
сравнение с эталонными видами и уточнение расчёта геометрии. См. GPU_RENDERING.md.

2026-10-13: эталонные RPCS3 screenshots показали высокую вертикальную траву.
Новая правка трёх scalar ALU операций точного шейдера восстановила её форму без
принудительного изменения W. Пользователь подтвердил вертикальную траву; два
живых GPU-кадра возле Хаксли проверены, длинных полос и наклонных лент нет.
Коррекция включена в обычную Windows RelWithDebInfo сборку; прямой запуск EXE
без GPU-флагов загрузил xenos-grass. StockGpu сохраняет штатный SDK для сравнения.
Строгая генерация, native build и пять тестов прошли. Общая семантика ALU Xbox 360
и остальные уровни этой проверкой не подтверждаются.

Аудит кампании: check_campaign.py выявляет XEX по сигнатуре независимо от
расширения, сравнивает источники и guest paths с manifest. Все 15 DLL уровней
имеют preferred image base 0x89000000; подключён только Saint-Lo. Штатные SDK
API регистрации/выгрузки изучены, но реальный переход ещё не проверен.
См. CAMPAIGN_READINESS.md. После добавления проверок инвентаризации проходят
6 parser/coverage tests и 2 native tests; это не доказывает прохождение миссий.

Версии: ReXGlue 0.10.0 / c94f5ebdcb3c9d1a460ca48e04f9758448f8d518;
LLVM 23.1.3 для приложения, Clang 19.1.5 в первой исходной сборке SDK;
Build Tools 17.14.41, MSVC 14.44.35207, Windows SDK 10.0.26100.0;
CMake 4.4.4; Ninja 1.13.2; Git 2.55.0.windows.3; Python 3.12.14;
Ghidra/XEXLoaderWV 12.1.3; Temurin JDK 21.0.12.101.
