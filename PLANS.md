# PLANS.md

## Objective
Sync the current `ultimate-merge.v2` branch with `main` and resolve merge conflicts while prioritizing the branch's own changes.

## Open questions
- (empty)

## Approved plan
- Merge `upstream/main` into the current branch.
- Resolve content conflicts by preserving local branch behavior where branch and main disagree, while incorporating non-conflicting upstream updates.
- Review each conflict for behavioral impact.
- Run lightweight verification commands after resolving; do not commit automatically.

## Implementation status
- [ ] Not started
- [ ] In progress
- [x] Done

## Decisions
- Local branch changes take priority in conflict resolution.
- No commit will be created unless explicitly requested.
- Keep the branch's unified `append_tcr` wipe-tower path instead of reintroducing upstream's deleted `append_tcr2` implementation.
- Combine the branch's preheat temperature override handling with upstream's post-process layer tracking for first-layer temperature selection.
- Skip empty cherry-picks when the target branch already contains an equivalent change.
- When cherry-picking multi-material mapping onto `ultimate-merge.v2`, preserve the branch's hotend placeholder support and sidebar behavior while switching tool-related placeholders to mapped output tool IDs.
- Orca-managed extruder mapping must also activate for non-SEMM FFF printers with exactly one physical extruder, so all filaments normalize to mapping `1`.
- Preview filament/tool legends should be derived from non-zero print statistics, not from raw viewer move IDs, so placeholder/default tool `0` does not create a fake extra filament row.
- In Preview `Filament` view, multiple rows may still be correct for single-extruder Orca-managed mapping because the rows represent logical filaments/colors, not physical toolheads.
- Preview color-change timing must guard against logical filament IDs larger than the physical extruder count, otherwise the sidebar can corrupt time output.
- Preset saving for inherited machine profiles must diff against the union of child/parent keys, otherwise newly introduced options like `use_physical_extruder_ids_only` are silently dropped when the parent lacks the key.
- G-code processing resets must clear `print_statistics` and reset `PrintEstimatedStatistics::Mode` by reference, otherwise stale or uninitialized time values can leak into filenames and preview timing.
- In Orca-managed mapping, logical filament/color switches that stay on the same physical extruder must remain as `;VT` metadata only; they must not generate a real `T` command or a wipe tower.
- The G-code importer must treat standalone `;VT` markers as logical filament switches for preview coloring, but without charging physical toolchange time or incrementing physical toolchange counts.
- `use_physical_extruder_ids_only` must be listed in `Preset::printer_options()`, otherwise the value may exist in the user JSON on disk but still be dropped when printer presets are loaded back into memory.
- The Prepare 3D canvas must use the same effective-tool-count logic as slicing/export; otherwise it can show a fake wipe tower for multi-color single-physical-extruder Orca-managed mapping even when the sliced preview and G-code are already correct.

## Handoff
- Agent: Codex
- Date: 2026-06-03
- Completed this session:
  - Fixed the 1-extruder Orca-managed filament mapping path in `src/slic3r/GUI/PartPlate.cpp` and `src/slic3r/GUI/Plater.cpp` by removing the `> 1 extruder` activation requirement.
  - Updated `src/slic3r/GUI/Tab.cpp` so toggling `Enable Orca-managed extruder mapping` refreshes the filament badges immediately.
  - Updated `src/slic3r/GUI/GCodeViewer.cpp` so preview filament/tool lists use non-zero print statistics instead of raw `used_extruders_ids`.
  - Updated `src/libslic3r/Preset.cpp` to use `deep_diff(...)` when saving inherited presets, so `use_physical_extruder_ids_only` persists even if the parent machine profile does not define the key yet, and guarded save paths where the parent preset lacks a vector option entirely.
  - Added `use_physical_extruder_ids_only` to the printer preset option whitelist in `src/libslic3r/Preset.cpp`, so the saved value is loaded back into the in-memory printer preset after restarting the app.
  - Updated `src/slic3r/GUI/GLCanvas3D.cpp` so the Prepare view suppresses the wipe tower proxy for Orca-managed mapping with one physical extruder unless a real tower is still required for timelapse or wrapping detection.
  - Fixed `src/libslic3r/GCode/GCodeProcessor.hpp` and `src/libslic3r/GCode/GCodeProcessor.cpp` resets so print statistics and mode times are actually cleared between runs.
  - Updated `src/libslic3r/GCode.cpp`, `src/libslic3r/GCodeWriter.cpp`, `src/libslic3r/GCode/GCodeProcessor.cpp`, and `src/libslic3r/Print.cpp` so 1-physical-extruder Orca-managed mapping uses physical tool IDs consistently, preserves logical `;VT` color markers, suppresses redundant same-tool `T0` commands, and no longer enables a wipe tower just because multiple logical colors exist.
  - Verified the result with `cmake --build build/arm64 --config Release --target all -- -j1` and `git diff --check`.
- Stopped at:
  - The working tree contains the printer-preset persistence fix, the G-code statistics reset fix, the preview/export compaction fixes, and the new virtual-only same-physical-toolchange path; the changes are not committed yet.
- Next step:
  - Restart Orca and verify end-to-end that the checkbox persists, the Prepare view no longer shows a fake tower for the managed 1-extruder case, and the sliced preview/G-code stay tower-free while preserving logical `;VT` color markers.
- Open blockers:
  - none
- Decisions made this session:
  - 1-extruder managed mapping should normalize all filament badges to `1`, but it should not force the edit button into a mapping menu with no real mapping choices.
  - The preview should ignore zero-usage fallback/default tool IDs introduced before the first virtual tool marker is resolved.
  - A single-extruder managed-mapping print can legitimately show several logical colors, but those logical color changes must not force a wipe tower or a real physical toolchange when they all map to the same nozzle.
  - Saving the checkbox was only half the fix; the printer preset loader also needed to recognize `use_physical_extruder_ids_only` as a first-class printer option.
  - The Prepare-view wipe tower must be gated by effective physical tool count, not raw logical filament count, to stay consistent with the actual slice/export result.

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
