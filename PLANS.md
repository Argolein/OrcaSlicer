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
- ~~Do not commit `PLANS.md` automatically as part of the branch sync.~~ **Superseded 2026-07-30:**
  always commit `PLANS.md` on this branch, together with the work it describes, so the record
  cannot drift from the diff.
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
- 2026-07-30 sync: the branch's `_travel_to_z`/`_spiral_travel_to_z` null-guard was dropped — upstream's `m_cached_extruder_idx` (default 0, refreshed on toolchange) removes the `filament()` dereference entirely and its default matches the branch's old fallback exactly. The crash fix is preserved structurally, not by the guard.
- 2026-07-30 sync: the branch's `wtwCone` removal from `WipeTowerWallType` stands; upstream's `wipe_tower_cone_angle` visibility line referencing `WipeTowerWallType::wtwCone` would not compile and was dropped. `prime_tower_width` stays editable under rib wall (branch behaviour) rather than upstream's `!have_rib_wall`.
- 2026-07-30 sync: upstream's new pre-heating block builder parsed `;VT` with `str >> fid` and silently ignored the branch's `;VT T<fid>` spelling. Patched the parser to skip an optional `T` rather than changing the branch's marker format, which is load-bearing for saved G-code and the strict-physical-tool-id path.
- 2026-07-30 sync: `_make_wipe_tower()` purge-volume loop — kept the branch's model verbatim (prime/purge split, `purge_in_prime_tower && SEMM` path with `filament_minimal_purge_on_wipe_tower` add-back, per-physical-nozzle tracking). Upstream's `NozzleStatusRecorder` carousel tracking, `prime_volume_mode` and per-filament `filament_prime_volume(_nc)` were dropped here. **Consequence:** H2C carousel printers keep the redundant-AMS-flush behaviour upstream fixed, and `prime_volume_mode` / `filament_prime_volume` / `filament_prime_volume_nc` exist as config options but do not affect wipe-tower planning on this branch.
- 2026-07-30 sync: upstream's `WipeTower2` else-branch and `WipeTowerIntegration::append_tcr2()` (283 lines) were dropped again per the standing unified-pipeline decision; `append_tcr2` has no remaining callers.
- 2026-09-27 sync: upstream `7a378d2fc4` re-synced `WipeTower` from BambuStudio (~2100 lines). WipeTower.cpp/.hpp were rebuilt from upstream's version and the branch deltas re-applied on top (wall filament override, category sync, `get_z_and_depth_pairs`, no stability floor in `plan_tower_new`, TPU zero-length guard, rib contour fix). Branch ports of BambuStudio features that upstream now ships natively (layer types, infill gaps, per-layer skip points) were replaced by upstream's versions.
- 2026-09-27 sync: upstream removed `WipeTower::set_filament_map`, which carried the branch's "no nc_depth for non-BBL printers" rule (e8ead44573). Re-ported as `WipeTower::set_nozzle_change_in_tower(bool)`, set by Print to `is_BBL_printer() || is_QIDI_printer()`; when off, the nozzle/extruder-change checks answer "same nozzle".
- 2026-09-27 sync: `_make_wipe_tower()` keeps the branch's structure and purge model and adopts upstream's WipeTower API (nozzle groups, accelerations, first-layer flow, per-slot purge tracking, 3-volume `plan_toolchange`, `generate_new`, skip mid-air final purge, exact footprint check). `append_tcr2`, `travel_to_tower_gap`, `transform_wt2_pt` dropped again; `tool_change` uses upstream's precomputed compacted Z with `get_active_z_offset` as its base.
- 2026-09-27 sync: the "cone" wall type stays removed; upstream's new `wtwCone` uses in `WipeTower2.cpp` and `WipeTowerEstimate.cpp` were deleted (the estimate outline never carries a cone base).
- 2026-09-27 sync: upstream's BambuStudio re-sync dropped three Orca tower-interface features from `WipeTower` (kept only in `WipeTower2`, which the unified pipeline never runs). Re-ported onto upstream's new code: the extra interface purge (`filament_tower_interface_purge_volume`, incl. the branch's support-filament extension; no depth reserved, as before) and the `enable_tower_interface_cooldown_during_tower` timing (layered on upstream's Contact-grid M109). The old 20 mm/s "layer after interface" slowdown was NOT re-ported: before the merge it lived in `finish_layer()`, which only measured extrusion length, so it never reached G-code — making it live would have changed output.
- 2026-09-27 sync: behavior change accepted from upstream, confirmed by Owner on 2026-09-27: the interface print temperature is now applied only in upstream's Contact-grid code, no longer at the toolchange into an interface layer (the old `tool_change_new` M109). The branch's GCode-side support-only preheat (`use_support_tower_interface_temp`) is unchanged.
- 2026-09-27 sync: accepted upstream defaults in the wipe tower: a sparse layer 0 is skipped in `tool_change` (the branch always printed it); the fake-tower position includes the rib offset (matches `append_tcr`); the pre-slice footprint estimate still sizes non-BBL printers with Type2 rules and the stability floor (over-reserves slightly; forcing Type1 would break upstream's estimate tests).
- 2026-09-27 sync: upstream's new `wait_for_temp_on_wipe_tower` (408db4b3b0) is WipeTower2-only; in the unified pipeline it did nothing, so its line is hidden in the printer tab (Tab.cpp), like the cone angle.
- 2026-09-27 sync: fixed a pre-existing branch crash exposed by upstream's new tests: non-BBL printers with `single_extruder_multi_material_priming` on skipped the initial `set_extruder` while `WipeTower::prime` produces nothing, so `process_layer` dereferenced a null filament. GCode.cpp now counts priming only when the tower returned priming lines (`wipe_tower_priming`).
- 2026-09-27 sync: upstream's re-sync dropped the branch's `m_is_multiple_nozzle` gate on `should_heating` in WipeTower, so every non-BBL toolchange got `M400` + `M104`. Restored as `s_IsBBLPrinter || m_is_multiple_nozzle` (BBL keeps upstream behavior).
- 2026-09-27 smoke test: CLI slicing of a 2-filament job (no 3MF) crashed. Two CLI-only causes, not the wipe-tower planner: (1) `filament_colour` is a project setting, not a filament preset option, so the CLI's per-filament merge left it at 1 entry while Print derives the filament count from its size — brim indexed `filament_map` past the end and the tower planned no toolchange. The CLI now pads `filament_colour` to the loaded filament count. The pre-merge CLI had the same gap but silently collapsed filament 2 into filament 1. (2) Upstream's CLI read-back `set_filament_maps(...)` hit the branch's GUI-only normalization (`wxGetApp().preset_bundle`); `PartPlate::set_filament_maps` now stores the maps as-is when there is no GUI app, as upstream does.
- 2026-09-27: PartPlate's filament-map helpers (`get_filament_maps`, `get_real_filament_maps`, `orca_managed_extruder_mapping_enabled`) read `wxGetApp().preset_bundle`, which is null in the CLI. They now fall back to upstream's raw-map behavior when there is no GUI app. Reproduced crashes: `--export-3mf` without `--slice`, `--slice` with `--export-3mf`, and a 3MF whose plate stores `filament_maps`. The GUI path is unchanged.
- 2026-09-27: the merged build crashed at GUI startup: upstream's rebuilt `MenuFactory::create_filament_action_menu` has no `if (init) return;`, and the branch's Extruder Mapping block read `plater()->sidebar()` while the Plater was still being constructed. The mapping check is now skipped on the init call. Also guarded `check_single_extruder_mixed_filament_risk` and `set_default_wipe_tower_pos_for_plate` against a missing GUI app (upstream code, GUI-only callers today). MERGE_NOTES now requires an app-start check after every sync.
- 2026-09-27 sync: upstream now builds with `-Werror` on Clang. Branch code must compile warning-free (first hit: an unused `this` capture in SnapmakerPrinterAgent.cpp).

## Handoff
- Agent: Claude Code
- Date: 2026-09-27
- Completed this session:
  - Synced `ultimate-merge.v2` with `upstream/main` (6a07853933): was 1214 behind / 87 ahead. Merge base 54dc5a2f1d.
  - Resolved 82 conflict blocks in 19 files: WipeTower.cpp (30), Print.cpp (10), WipeTower.hpp (6), GCode.cpp (4), GLCanvas3D.cpp (4), PartPlate.cpp (4), GCode.hpp (3), and 1-2 each in Preset.cpp, PrintConfig.cpp, ConfigManipulation.cpp, Plater.cpp/.hpp, GUI_Factories.cpp, GUI_App.cpp, GCodeViewer.cpp, Tabbook.hpp, TreeSupport.hpp, OrcaSlicer.cpp, CMakeLists.txt. Per-block notes were kept in the session scratch only.
  - Unified wipe-tower pipeline kept on top of upstream's BambuStudio re-sync of `WipeTower`; see the 2026-09-27 entries in ## Decisions.
  - Deps rebuilt (upstream added Assimp, SLVS, FFmpeg and patched wxWidgets/TBB/Python). Full arm64 macOS build clean, with upstream's new Clang `-Werror`.
  - Tests (`-T`, first time on this branch): 1299 of 1309 pass. Two crashes (a pre-existing branch bug) and one regression were fixed.
- Remaining test failures (10), tests left untouched:
  - Expected, because upstream's tests pin WipeTower2 / cone behavior the branch removed: #786, #833 (cone), #1120, #1200 (WipeTower2 priming), #1177, #1211 (tower temperature wait), #1240 (`;HEIGHT` formatting), #1142 (rib width vs square footprint; likely, not confirmed).
  - Branch feature: #406 (H2C Hybrid slots) fails because `normalize_filament_maps` (ff9a5dd81c) maps filament indexes >= physical extruder count to extruder 1.
  - Unexplained: #1156 (custom G-code motion limits restored; acceleration clamped to 1500 by machine limits; code matches upstream). May already have failed before the merge.
- Stopped at:
  - Merge committed and pushed to origin/ultimate-merge.v2 on Owner's OK (2026-09-27).
- Next step:
  - Smoke test done 2026-09-27 via CLI (two cubes, two filaments): Bambu X1C, Prusa XL 5T (+ interface features, + priming), Prusa CORE One MMU3, Custom MyToolChanger, Custom MyKlipper with Orca-managed mapping all slice; tower toolchanges, T commands and `;VT` markers as expected. GUI checked the same day after the startup fix: the XL 5T two-filament project slices in the app window and the filament menu opens.
- Open blockers:
  - none

## Handoff (previous)
- Agent: Claude Code
- Date: 2026-07-30
- Completed this session:
  - Synced `ultimate-merge.v2` with `upstream/main` (54dc5a2f1d): was 433 behind / 86 ahead. Merge base 6fda82476d.
  - Resolved 51 conflict hunks across 18 files: GCode.cpp (18), Print.cpp (6), Plater.cpp (4), GCodeWriter.cpp (4), OptionsGroup.cpp (3), Tab.cpp (2), WipeTower.{cpp,hpp} (2+2), GCodeProcessor.cpp (2), and one each in GCode.hpp, GCodeProcessor.hpp, PrintConfig.cpp, TreeSupport.cpp, ConfigManipulation.cpp, Field.cpp, PartPlate.cpp, TextInput.hpp, NetworkAgentFactory.cpp.
  - Branch features preserved: filament max-accel clamp, short-travel acceleration, Orca-managed mapping / mapped tool ids + `;VT`, unified wipe-tower pipeline (no `append_tcr2`), rib-wall toggles, bed-type Z offsets, tower-interface preheat (support-filament-only), tree-support bottom gap, Prusa/Snapmaker agents, QIDI flag, OptionsGroup null-safety.
  - Upstream features adopted on top: per-variant config columns (`get_filament_config_index` / `get_nozzle_config_index`), 2-arg `toolchange(filament_id, nozzle_id)`, nozzle/hotend/variant placeholders, `m_cached_extruder_idx`, filament-volume maps in `full_config`, printer-agent plugin system, wipe-tower printable-height clamp + `has_filament_switcher`, per-variant ramming/pre-cooling filament options, `top_base_interface_layers`.
  - One decision deferred to the user: the `_make_wipe_tower()` purge-volume model — chose "keep branch, drop upstream's" (see Decisions).
  - Deps rebuilt (upstream added `python3`/`pybind11`/`wxInspector`; 15m26s) and full arm64 macOS Release build clean — 743/743 targets, 0 errors, 11m24s.
  - Committed as `2f1be0201b` (`Merge upstream/main into ultimate-merge.v2`); branch now 0 behind / 87 ahead of upstream. Nothing pushed.
- Stopped at:
  - Merge committed and building clean. Not pushed — MERGE_NOTES requires explicit OK.
- Next step:
  - Runtime smoke test before pushing: slice a multi-material plate on a non-BBL printer (exercises the unified wipe-tower path + `_travel_to_z` preamble), and check a toolchange's `;VT`/T output under Orca-managed mapping. This sync also pulls in upstream's Python plugin system (pybind11 + bundled Python 3), a larger surface than a typical sync.
- Open blockers:
  - none

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
