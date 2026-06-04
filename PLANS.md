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
- Do not commit `PLANS.md` automatically as part of the branch sync.
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
- During the next `upstream/main` sync, keep the branch's strict-physical filament statistics path and thread upstream's `tool_ordering()` fallback through it, so non-wipe-tower prints still report tool changes correctly.
- During the same sync, keep the branch's wipe-tower rib-wall and filament selector toggles in `ConfigManipulation.cpp`; the newer upstream wipe-tower option visibility changes are additive, not replacements.

## Handoff
- Agent: Codex
- Date: 2026-06-04
- Completed this session:
  - Fetched all remotes and confirmed `ultimate-merge.v2` is 100 commits behind and 72 commits ahead of `upstream/main`.
  - Merged `upstream/main` into `ultimate-merge.v2` and preserved the existing branch history with a merge commit instead of a rebase.
  - Resolved the `src/libslic3r/GCode.cpp` conflict by keeping the branch's strict-physical filament statistics path and wiring upstream's `tool_ordering()`-based toolchange fallback through both the strict and normal code paths.
  - Resolved the `src/slic3r/GUI/ConfigManipulation.cpp` conflict by keeping the branch's wipe-tower rib-wall and filament-selector toggles while also retaining upstream's newer wipe-tower option visibility changes.
  - Finalized the sync as merge commit `9923ade284` (`Merge remote-tracking branch 'upstream/main' into ultimate-merge.v2`).
  - Verified there are no remaining conflict markers, `git diff --check` passes, and the branch is now `0` behind / `73` ahead of `upstream/main`.
- Stopped at:
  - The branch sync is complete; only the local `PLANS.md` handoff update remains uncommitted in the working tree.
- Next step:
  - If desired, push `ultimate-merge.v2` and/or run a build or app-level verification on top of merge commit `9923ade284`.
- Open blockers:
  - none
- Decisions made this session:
  - The sync should preserve the branch's strict-physical filament statistics behavior and only layer upstream's non-wipe-tower toolchange counting on top.
  - The sync should preserve the branch's wipe-tower UI toggles for rib walls and per-role filament selectors; upstream's visibility updates should be merged around them, not replace them.

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
