# Native title relaunch

Verified runtime result, 2026-10-15: user completed Saint-Lo from the restored
saint_lo_final checkpoint and confirmed Island loaded with working player
control after native relaunch. Parent native-20261015-165906-292.log records
SetLaunchData/LaunchTitle at 17:04:25.622 host time and queued handoff at
17:04:25.830. Successor native-20261015-165906-292-relaunch.log records
RESTORED 664 at 17:04:28.574, original GetLaunchData success at 17:04:28.634,
Island entry reason=1/result=1 and LOAD_RESULT=0 at 17:05:36.016. No FATAL
was found. CIM confirms only successor PID 34696 exists; parent PID 20020
has exited. Userdata remained saint_lo_repeat_01, DoF remained 35. This is one
verified campaign transition, not full Island completion or campaign coverage.

Runtime update 2026-10-15: native-20261015-131421-759.log at 13:48:14 host time
records SetLaunchData success (664 bytes), LaunchTitle default.xex flags 0,
then the resolved path \Device\Harddisk0\Partition1\default.xex. The initial
game:-only policy rejected this legitimate request. The exact verified alias
is now accepted; other partitions and multiplayer images remain rejected by
regression tests. Strict codegen/build and all 13 tests passed again:
logs/title-device-path-{build,tests}-console.log. The user had been asked to
exit at a checkpoint, so this request alone does not prove mission completion.

The original campaign has two verified paths that send 664 opaque bytes to
XamLoaderSetLaunchData before XLaunchNewImage. The pinned SDK stores the bytes
and launch path but ends the title without launching the next image. Instruction
evidence and executable hash are recorded in CAMPAIGN_READINESS.md.

The authored host implementation uses the SDK's OnGuestThreadExit callback.
Only game:\default.xex or the verified SDK device alias
\Device\Harddisk0\Partition1\default.xex with flags 0, launch_data_present and exactly 664 bytes
qualifies. Ordinary exits, empty launch requests, multiplayer and other images
do not relaunch. `--cod3_relaunch=false` disables the feature for comparison.

The next native cod3pc process inherits only an anonymous pipe reader and a
wait handle for the original process (explicit Windows handle allowlist).
Before constructing ReXApp or creating its game window, it waits up to 60 seconds
for the original process to exit, reads the bytes, and closes both handles.
There can briefly be a waiting second process, but never a second initialized
game window/runtime. No game-state editing, temporary launch-data file, or
substituted guest function is involved.

OnPostSetup restores the bytes into the same XamModule LoaderData used by
the pinned Set/GetLaunchData APIs, before original guest startup. The game
itself interprets the data and chooses the next map. The userdata path and
original CLI settings are preserved; current DoF is forwarded. Initial `--cl`
(including debug devmap) is cleared to avoid replaying the previous map.
The successor gets a separate `-relaunch` log. Loader/transport failures are
logged; they do not change the guest's termination behavior.

Validation 2026-10-15: strict codegen, native build, 10 Python tests and three
native tests passed. native_title_handoff covers accepted/rejected launch paths,
flags and sizes, Windows quoting (including empty strings, Unicode, quotes and
trailing backslashes), CLI preservation, and real worker/successor processes.
The successor verifies all 664 synthetic bytes and waits for worker exit; the
worker checks that success cannot be signaled while it remains alive.
Private logs: title-relaunch-build-console.log, title-relaunch-tests-console.log.

The transport-only result was later followed by the real original-game
Saint-Lo transition recorded above; it is no longer just a transport test.
Thirteen SP modules remain unintegrated, so relaunch alone does not enable the
entire campaign. No code from Project 1944 was copied.

Manual check: user restored the copied Island checkpoint and selected restart
level; it loaded again. native-20261015-130958-439.log contains original Island
entry/load success, but no SetLaunchData/LaunchTitle/relaunch request. PID 38132
was unchanged. This is an in-process mission restart, not validation of the
new title handoff. After the user authorized closing this test, a fresh isolated
saint_lo_end profile was opened through the normal menu to capture a genuine
late-Saint-Lo checkpoint. That checkpoint does not exist yet.

Repeated helper initialization also exposed PATH growth from repeated vcvars
imports. Enter-Toolchain.ps1 now deduplicates PATH and reuses the already imported
x64 environment from the same Visual Studio installation. Three consecutive
imports retained PATH length 3713 and identical INCLUDE; private report
reports/toolchain-idempotence.json. A sequential build plus Tests run passed.
