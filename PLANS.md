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
- 2026-11-16 sync: upstream's IMEX (IDEX/IQEX, gated on `is_imex`) has its own logical-to-physical heater map. It coexists with Orca-managed mapping: `GCodeWriter::set_temperature` and `OozePrevention::pre_toolchange` take the IMEX map when `is_imex`, else `get_output_tool_id` / `get_toolchange_id`. Upstream's new "switch off unused heaters" check compared logical ids, which would switch off a physical extruder another mapped filament still needs; it now compares mapped tool ids.
- 2026-11-16 sync: upstream's "joined" Precise-Z sliver layers (#17204) adopted in the unified tower path; `joined_tower_z()` uses `get_active_z_offset` as its base, like `m_compacted_tower_z`. `plan_toolchange` calls use `wipe_tower_print_z`.
- 2026-11-16 sync: `tower_height_tag()` and GCode's `m_sparse_layers_combined` dropped with upstream (#17199): the tower now writes the printer's tag dialect itself, so the helper never fired. The branch's duplicate `s_IsBBLPrinter` assignment before `generate_new()` was removed; upstream sets it at the top of `_make_wipe_tower()`.
- 2026-11-16 sync: upstream's `WipeTower2`-only changes stay out of the unified pipeline (`append_tcr2`, Type2 `_make_wipe_tower` branch). Checked: the restore-layer-Z lift and the Klipper M400 fix also reached `append_tcr` / `WipeTower.cpp`. Not ported, by Owner's decision (2026-11-16): upstream's final unload for single-extruder MMU when the tower stops below the last layer (#17496); the unified path keeps skipping it, as before.
- 2026-11-16 sync: CoolingBuffer bit collision — upstream's belt-band tags took `1 << 21/22`; the branch's internal-perimeter tags moved to `1 << 23/24` (in-memory only).
- 2026-11-16 sync: `enable_arc_fitting` became an enum (Disabled / Enabled / Sparse infill only). The branch's "active Z offset disables arc fitting" rules in ConfigManipulation.cpp and Field.cpp now read and write `ConfigOptionEnum<ArcFittingMode>`; a bool lookup would silently never fire.
- 2026-11-16 sync: Snapmaker AMS matching — Owner no longer uses Snapmaker printers, so the branch's matcher (`resolve_tray_info_idx`, sub-type-preserving `combine_filament_type`) was dropped and upstream's agent (#16300, with tests) taken verbatim.
- 2026-11-16 sync: PartPlate `estimate_wipe_tower_polygon` keeps the branch's rotated-footprint clamp and runs upstream's re-place-clear-of-objects step (#16728) on the branch's per-axis anchor bounds, with the rotated footprint polygon as the overlap shape.
- 2026-11-16 sync: Plater background apply follows upstream's order (device filament info before `apply`) but keeps `full_config_for_current_mapping_mode`.
- 2026-11-16 sync: fixed a pre-existing branch mismatch exposed by upstream's new tests: `Print::_make_wipe_tower()` plans every non-BBL, non-QIDI tower from `all_extruders().back()`, but `GCode::_do_export` started from there only for Type 2. A non-BBL Type 1 print threw "append_tcr was asked to do a toolchange it didn't expect". The G-code now uses the tower's rule (`! is_bbl && ! is_QIDI`). QIDI Type 2 now starts at `first_extruder()`, matching its tower (untested: no QIDI case).
- 2026-11-16 sync: upstream's "switch off unused heaters" (109d09e0b8) treats another filament on the same heater as still needing it unless that filament finished on an earlier layer; otherwise a mapped top layer got `M104 S0` followed straight by `M109`. Mapped smoke output matches the pre-merge app again.
- 2026-11-16 sync: `current_hotend` / `next_hotend` placeholders follow upstream's new rule (ee26e94170, H2D fix): -1 on every BBL printer without a dynamic nozzle map, the extruder index elsewhere. The old rule (only multi-nozzle H2C / X2D got -1) was upstream code the branch never changed, so it was not kept.
- 2026-11-16, Owner's decision: `normalize_filament_maps` (Print.cpp and PartPlate.cpp) normalizes only under Orca-managed mapping; every other printer keeps its own or manual filament map, as upstream. `Extruder::extruder_id()` keeps the branch's form (filament map only under Orca-managed mapping, else the filament id): upstream's form was tried and reverted — it cut Prusa MMU3 tool changes from 102 to 2 and failed 13 filament-assignment tests. So the hotend-placeholder test (manual map 2,1 on a non-BBL printer) stays a known failure.
- 2026-11-16: `deps/Assimp/Assimp.cmake` forces the bundled minizip (`-DASSIMP_BUILD_MINIZIP=ON`); a Homebrew minizip found through pkg-config broke the deps build.
- 2026-11-16: tried and reverted: sizing the pre-slice tower estimate by Type1 rules for every printer. It fixed no test (the lid-clearance one still fails) and broke two upstream estimate tests, so the 2026-09-27 trade-off stands.
- 2026-11-16 sync: upstream test "A support-only change on a belt purge print matches a fresh slice" is flaky here (2 of 5 runs fail on one binary; header machine limits and time estimate differ). Not investigated; not known whether upstream's own build flakes too.
- 2026-11-16 sync: fixed a pre-existing branch bug (since 62729b08ae): `diff_with_missing_parent_options` used `deep_diff()`, whose per-column `key#i` entries `config.option()` cannot resolve, so saved user presets with a parent silently dropped per-column vector changes; a disabled filament override crashed on save. It now uses `config.diff()` (whole keys, as upstream) and keeps the add-back of keys the parent lacks.
- 2026-11-16 sync: the wipe-tower filament end G-code placeholder `filament_extruder_id` (new upstream code in `append_tcr`) uses `get_toolchange_id`, like the other tool-facing placeholders.
- 2026-11-16, Binary G-code (Owner's request): printer option `binary_gcode` (default off) saves and uploads Prusa `.bgcode`. Owner chose: libbgcode (commit 5041c09, as PrusaSlicer 2.9.4) and heatshrink 0.4.1 bundled in `deps_src/`; export and upload only (no `.bgcode` viewer import, convert menus or file association); on in the Prusa profiles for the printers PrusaSlicer enables (every Buddy printer, off for MK3S; CORE One INDX inherits it).
- Binary G-code is converted at the output boundary (`BackgroundSlicingProcess::finalize_gcode` / `prepare_upload`, after post-processing scripts), not in `GCodeProcessor` like PrusaSlicer: the preview keeps reading the ASCII temp file, so the viewer needs no binary support. The CLI still writes ASCII `plate_N.gcode`.
- The vendored libbgcode converter is patched (marked `OrcaSlicer:`) to accept `generated by OrcaSlicer` and `CONFIG_BLOCK_START/END`, and to match metadata keys whole (upstream's prefix match took `temperature_vitrification` for `temperature`). Printer-metadata keys with other Orca names (`temperature`, `bed_temperature`, `fill_density`, `support_material`, `ironing`) stay empty; the Buddy firmware reads none of them and checks the model through `M862.3`.

## Handoff
- Agent: Claude Code
- Date: 2026-11-16
- Completed this session:
  - Synced with upstream `main` (e0f7f0b7ee): was 1931 behind / 94 ahead, merge base ae27a795b6. 86 conflict blocks in 24 files resolved; the 2026-11-16 entries in ## Decisions record every non-obvious choice.
  - Upstream features adopted around branch features: IMEX (IDEX/IQEX), joined Precise-Z tower layers, belt printers, arc fitting modes, prime tower relocation, standby heater shut-off, Klipper M400 reduction.
  - Fixed during the sync: non-BBL Type 1 tower start-filament mismatch (export threw), preset save dropping per-column values (crash in a new test), mapped heater shut-off, PrusaPrinterAgent on the new agent API, arc fitting enum in GUI rules. Snapmaker agent taken from upstream (Owner no longer uses Snapmaker).
  - Deps rebuilt in the worktree (pkg-config hidden from Homebrew, see MERGE_NOTES); arm64 build clean.
  - Tests (after the follow-up commit): 3252 of 3268 pass. 16 failures, all deliberate or known branch differences: Cone wall (3), WipeTower2-only behaviour (6: tower temperature wait / priming, Klipper planner flush, Type2 lift), MMU final unload kept skipped, nozzle-change section off on non-BBL, merged-layer height tag, rib width vs depth, tower estimate vs actual (lid clearance), custom G-code motion limits, hotend placeholders with a manual map (branch `Extruder::extruder_id`). Flaky: "A support-only change on a belt purge print matches a fresh slice" (passes on some runs).
  - CLI smoke (`.workflow/scratch/smoke`, 6 setups) vs the pre-merge binary: all slice; identical tool/temperature output except upstream's Klipper M400 removal and 1 s preheat timing hints on the Prusa XL. App starts with a scratch data folder.
- Stopped at:
  - Merge (36461ced1a) and follow-up commit pushed; `ultimate-merge.v2` on origin fast-forwarded on Owner's OK.
- Next step:
  - Owner fast-forwards the local `ultimate-merge.v2` in the main checkout (`git pull --ff-only origin ultimate-merge.v2`).
- Open blockers:
  - none

## Handoff (previous)
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

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
