# Other native CoD3 ports: transition source review

Reviewed 2026-10-15 against current GitHub default-branch commit IDs, fetched
through the GitHub API. Authored host sources only; no game binaries or generated
game code were imported. No implementation was copied into cod3pc.

## CoD3Recomp

Revision: 73e8db563321ad10036005197ddb56b4fa68e483.
Source: [CoD3Host/kernel_xam.cpp](https://github.com/FrankHUN88453/CoD3Recomp/blob/73e8db563321ad10036005197ddb56b4fa68e483/CoD3Host/kernel_xam.cpp#L742).

Relaunch writes outgoing launch bytes to launchdata.bin under its saves root
(or temporary directory), constructs a successor environment with
COD3_LAUNCHDATA, removes first-run scripted-map/input settings, and uses
CreateProcessW to start the executable again. XamLoaderLaunchTitle calls this
for a default.xex target and then ExitProcess(0). The successor reads and
removes the handoff file. Thus this is explicitly another process, not an
in-process map switch. The inspected path contains no readiness handover
waiting for the next window. This is source behavior, not a manual visual test.

No top-level authored-code LICENSE/COPYING appeared in the checked root tree,
and GitHub did not return a license object. Source review does not grant an
assumed copying license; no code was copied.

## Project 1944

Revision: 62ab3b08e4db1a5f21fb0c13b49d4a3ebd004812 (unchanged from prior review).
Source: [integration/pc-controls/title_relaunch.cpp](https://github.com/zenit2893-cmyk/project-1944/blob/62ab3b08e4db1a5f21fb0c13b49d4a3ebd004812/integration/pc-controls/title_relaunch.cpp#L230).

Writes original launch bytes to a file, starts another native executable via
CreateProcessW, passes its launch-data path and the previous PID, and restores
the bytes into the original startup path. A marker links old/new PIDs for the
launcher. Despite the argument's name cod3_relaunch_wait, PreviousInstance only
obtains the prior PID; the startup code inspected does not wait for its exit.

The visible-transition improvement is a named ready event: the parent keeps
its previous window/last frame while the child starts, then waits for either
that event, child failure, or a 30-second timeout. NoteGameLoopRunning starts
a detached thread that sleeps 800 ms and signals the event. The parent then
continues into the original termination path. Two processes can overlap.
The readiness signal is a game-loop-plus-delay heuristic, not verified GPU
presentation of a new frame. Therefore same-process/same-window seamless
transition is not implemented by this path either, even if the desktop gap
is hidden. No manual visual test of this project was performed in this review.

Authored license: BSD-3-Clause, independently reread through GitHub API. Actual
source reuse would require its notices; only architectural observations were
recorded here.

## Consequences for cod3pc

| Approach | PID | Window behavior | Current source finding |
|---|---|---|---|
| CoD3Recomp | New process | Old process exits after spawning successor | Functional title reboot |
| Project 1944 | New process, overlap possible | Retain last old frame until heuristic ready event | Visual handover across processes |
| Current cod3pc | New process; child waits for old exit before runtime/window | Old window closes first | Verified campaign handoff; visible gap |
| User requirement | Same process | Preserve native window while replacing title runtime | Still requires SDK lifecycle work |

Both external projects corroborate the original launch-data/reboot contract;
neither supplies a ready implementation of clean ReXGlue in-process teardown.
Project 1944 suggests a possible *visual* improvement, but adopting it would
not satisfy the user's explicit same-process requirement and would remove our
current no-overlapping-runtime property. Keep these as separate engineering
choices; do not silently substitute visual handover for in-process support.

For in-process support the concrete unresolved issue remains three live old
job workers plus full GPU/audio/runtime retirement, recorded in
SEAMLESS_TRANSITIONS.md. Improve visibility only if separately chosen; do not
change the current runtime in response to this research alone.

Private source evidence is under reports/cod3recomp-current-* and
reports/project1944-current-title_relaunch.cpp. Git tracks this authored review,
not the downloaded code. Documentation-only change; no runtime tests rerun.
