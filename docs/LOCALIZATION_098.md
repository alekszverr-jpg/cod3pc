# Russian 098 asset installation

2026-10-16: installed the user-provided Russian package from
`E:/PS2/outputs/cod3_x360_port_098_2026-10-16/Call of Duty 3 Xbox 360`
into private `assets/game`. All 552 files (6,216,645,241 bytes) were checked
against the package manifest with SHA-256 before and after copying. Compared
with our previous asset tree, 31 files differ. Source files were not modified.
The previous complete tree is retained in private
`reports/localization-098/previous-game`; userdata and checkpoints were preserved.

## Executable compatibility

Independent Ghidra 12.1.3/XEXLoader exports of all 16 initialized memory blocks
have identical addresses, sizes and execution flags. Reviewed default.xex hashes:

- Original: `2944eec7d1231ad6798b5f9f8adf8855f5e489296b22eab45b27a577cee23692`.
- Russian objective filter: `b176596e2f206e6fdf0d3cdae10c44957c3da559c2b838f3dfd1d8f302edd23b`.
- Installed revision 05: `e057ffcd129af913b9c7334c847f749115fa6a4e3c0a9edacdd2db728f003682`.

Original versus Russian filter: only instruction 0x824B5EAC differs,
`4198015c` (`blt cr6,0x824b6008`) becomes `409a015c`
(`bne cr6,0x824b6008`). This retains the branch target and function boundaries
while admitting extended characters in objective text. All other loaded bytes
match. The native generator must consume this revised instruction.

Revision 05 versus the filter revision: only eight bytes at 0x82068D98 in
`.rdata` differ: `4c6f6164696e6720` becomes `c7e0e3f0f3e7eae0`.
This changes the equal-length startup string from `Loading ...` to
`Загрузка...`; the trailing dots and terminator are unchanged. All code matches
the reviewed filter revision. Its `.text` SHA-256 is
`39581daa5fa647c7439ec7ce0201e94aa109687633c446be954e545532c08620`.

Saint-Lo and Island executable hashes remain unchanged. Build guards still
reject unknown executables. Multiplayer assets were copied as part of the
original directory tree; native multiplayer support remains outside scope.

The source package was updated during inspection, so both Russian XEX versions
were reviewed separately before accepting the installed revision. Private
evidence is in `reports/localization-098` (copy-verification.json,
installation.json, xex-comparison.json, loading05-comparison.json and section
exports) and `logs/ghidra-localization-*.log`. The reusable export script is
`tools/ghidra/ExportCod3Image.java`. No original/derived game bytes are committed.

Validation commands: `./tools/Build.ps1 -Stage Build` (strict codegen with
`--ignore-stamp`, native build), then `./tools/Build.ps1 -Stage Tests`.
Both passed on 2026-10-16: 10 Python tests and four native tests. Generated
`cod3pc_recomp.26.cpp` contains `bne cr6,0x824b6008` as expected; no generated
code was edited by hand. One normal-menu runtime was launched with DoF 35%.
Visual acceptance of this translation, line wrapping and button prompts needs
a real menu/gameplay check; file comparison alone does not establish it.

2026-10-16 runtime feedback: the user confirmed the translation works and
provided a Saint-Lo cutscene screenshot with readable Russian subtitles.
This confirms the shown subtitle context; all missions, menus and button
prompts have not yet been visually reviewed.
