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
- 2026-07-07 sync: upstream's "multi-variant" refactor (`1c8c7820c8`) made speed/acceleration/jerk configs per-extruder vectors (`ConfigOptionFloatsNullable`); keeping the branch's scalar types is impossible (the merged `NOZZLE_CONFIG` macro calls `.get_at()`, which only exists on vector options). Adopt upstream's vector types and re-port branch features on top.
- The branch's `rounded_filament_acceleration(double)` helper is type-agnostic; preserve the filament max-accel clamp by feeding it `NOZZLE_CONFIG(...)` per-extruder values instead of upstream's inline `floor(x+0.5)`.
- Keep `travel_short_distance_acceleration` a scalar (`ConfigOptionFloat`) option — it stays global while its siblings are per-extruder. Consequence: its "exceeds machine max" advisory warning in `Print.cpp` is dropped (upstream's `get_abs_value_at` throws on scalar options); the short-travel slicing behavior itself is unchanged.
- The branch's PrintConfig.cpp block (lattice/lightning/infill_anchor/accel defs) was a byte-identical relocated duplicate of options upstream already defines; drop it and keep upstream's correctly-typed copies, preserving only the branch-unique `travel_short_distance_acceleration`.
- CoolingBuffer `set_current_extruder` became 2-arg (extruder_id, nozzle_id) upstream; the branch's extra single-arg `set_current_extruder(get_toolchange_id(...))` calls in GCode.cpp are superseded and removed.

## Handoff
- Agent: Claude Code
- Date: 2026-07-07
- Completed this session:
  - Synced `ultimate-merge.v2` with `origin/main` (== `upstream/main`, b2adfb5c13): was 111 behind / 81 ahead.
  - Merged `origin/main` with a merge commit (no rebase); resolved 18 conflict hunks across 9 files: GCode.cpp, GCode/CoolingBuffer.cpp, GCode/GCodeProcessor.cpp, Preset.cpp, Print.cpp, Print.hpp, PrintConfig.cpp, PrintConfig.hpp, slic3r/GUI/Tab.cpp.
  - Core decision: adopted upstream's per-extruder multi-variant speed/accel/jerk config model and re-ported branch features on top (filament max-accel clamp, short-travel accel, curve smoothing, consistent-surface cooling + initial_layer_fan_speed, multi-material VT mapping, QIDI flag). See new entries in ## Decisions.
  - Verified: no conflict markers remain; full arm64 macOS Release build succeeds in ~8 min with 0 errors (`build/arm64/.../OrcaSlicer.app`).
  - Finalized as merge commit `193f12ba53` (`Merge origin/main into ultimate-merge.v2`). Branch now 0 behind / 82 ahead of origin/main. Nothing pushed.
  - Post-merge crash fix `58cbe3ebb7`: slicing crashed (EXC_BAD_ACCESS in GCodeWriter::_travel_to_z via GCode::preamble) on non-BBL printers. Upstream's per-extruder _travel_to_z derefs filament()->id(), but filament() is null until the first toolchange and init_extruder() runs only for BBL printers before preamble. The branch's get_active_z_offset() makes preamble do a real (non-zero) silent Z move — unlike upstream's z_offset.value==0 which skips it — so non-BBL printers deref a null filament(). Fixed by null-guarding filament() in _travel_to_z()/_spiral_travel_to_z() (fall back to filament 0; preamble output is discarded so no G-code change). Rebuilt clean (0 errors).
- Stopped at:
  - Sync complete and committed. Only this `PLANS.md` handoff update remains uncommitted in the working tree.
- Next step:
  - Optional: run-time/app verification of acceleration behavior (per-extruder), short-travel accel, and cooling; then push `ultimate-merge.v2` if desired.
  - Note the one intentional behavioral reduction: the `travel_short_distance_acceleration` "exceeds machine max" advisory warning in Print.cpp is dropped (option kept scalar). Slicing behavior unchanged.
- Open blockers:
  - none
- Decisions made this session:
  - See the 2026-07-07 entries added to ## Decisions above.

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
