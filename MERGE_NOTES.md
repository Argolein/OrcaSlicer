# MERGE_NOTES.md — syncing `ultimate-merge.v2` with upstream

This is a long-lived **integration branch**: it stacks many feature branches on top of
OrcaSlicer upstream and is re-synced with `main` periodically. The same conflicts recur
each sync. This file is the canonical playbook so each sync doesn't re-derive everything.

`origin` (Argolein fork) `main` is kept identical to `upstream` (OrcaSlicer) `main` —
merging either is equivalent.

## Sync workflow

```bash
git fetch origin upstream
git merge --no-commit --no-ff origin/main      # or upstream/main — same commit
# resolve conflicts using the tables below
# then, BEFORE committing:
grep -rn '^<<<<<<< \|^>>>>>>> ' src/            # must be empty
./build_release_macos.sh -x -s -a arm64        # must build clean (arm64 macOS)
git commit                                     # do NOT push without explicit OK
```

Rule of thumb: **preserve branch behavior where branch and upstream disagree; take
upstream's non-conflicting updates.** When upstream refactors an API the branch's
feature rides on, adopt the new API and re-port the feature on top of it (do not revert
the refactor — other files already depend on it and it will not compile).

## Branch features → the files / config they own (protect these)

| Feature | Owns / touches | Config keys |
|---|---|---|
| Filament max-accel clamp | `GCode::rounded_filament_acceleration()` in GCode.cpp | `enable_filament_acceleration_limit`, `filament_max_acceleration` |
| Short-travel acceleration | GCode.cpp `travel_to`, Print.cpp, Tab.cpp, Preset.cpp | `travel_short_distance_acceleration` (**scalar** `ConfigOptionFloat`) |
| Curve smoothing (SuperSlicer port) | PrintConfig.*, PrintObject*.cpp | `curve_smoothing_precision/cutoff_dist/angle_convex/angle_concave` (scalar) |
| Consistent-surface cooling | CoolingBuffer.cpp (`AdjustableFeatureType`), Tab.cpp | `cooling_slowdown_logic`, `cooling_perimeter_transition_distance` |
| Multi-material / Orca-managed mapping | GCodeProcessor.cpp, GCode.cpp, GCodeWriter.* | `use_physical_extruder_ids_only`; `m_pending_logical_filament_id`, `;VT` metadata |
| Wipe tower overhaul | WipeTower*.cpp, GCode.cpp, ConfigManipulation.cpp | unified `append_tcr` (no `append_tcr2`); rib-wall + per-role filament toggles |
| Bed-type-specific Z offsets | GCode.cpp (`get_active_z_offset`) | (see `_travel_to_z` note below) |
| QIDI printer flag | Print.hpp | `m_isQIDIPrinter` (keep next to `m_isBBLPrinter`) |
| Printer agents | slic3r/GUI agents, MoonrakerPrinterAgent.cpp | Prusa filament sync, Snapmaker AMS matching |

## Recurring conflict resolutions

**Per-extruder "multi-variant" model (upstream `1c8c7820c8` + follow-ups).** Speed /
acceleration / jerk configs are now per-extruder vectors (`ConfigOptionFloatsNullable`,
`ConfigOptionFloatsOrPercentsNullable`). Access them via the `NOZZLE_CONFIG(x)` macro
(`m_config.x.get_at(cur_extruder_index())`) or `get_abs_value_at(key, cur_extruder_index())`.
`.value` / `.get_abs_value()` **will not compile** on these — `.get_at()` is vector-only.
Affected keys: `default/outer_wall/inner_wall/top_surface/initial_layer/bridge/travel/
sparse_infill/internal_solid_infill_acceleration`, all `*_jerk`, `default_junction_deviation`,
`bridge_speed`, `internal_bridge_speed`, `initial_layer_speed`, `initial_layer_infill_speed`.

- Keep the branch's `rounded_filament_acceleration()` clamp by feeding it the per-extruder
  value: `rounded_filament_acceleration(NOZZLE_CONFIG(outer_wall_acceleration))` — replace
  upstream's inline `(unsigned int) floor(x + 0.5)`.
- `travel_short_distance_acceleration` and `curve_smoothing_*` stay **scalar** (`.value`);
  keep them out of the per-extruder `get_abs_value_at` lists (that call throws on scalars).
- Print.cpp motion check → take upstream's `check_extruder(extruder_id)` lambda.
- PrintConfig.cpp: watch for **relocated duplicate** option blocks (branch defined some
  options in a second location). Keep upstream's correctly-typed copy; delete the branch
  duplicate; preserve only branch-unique options (e.g. `travel_short_distance_acceleration`).

**CoolingBuffer** — `set_current_extruder(extruder_id, nozzle_id)` is 2-arg upstream; drop
any old 1-arg `set_current_extruder(get_toolchange_id(...))` calls (superseded).

**GCodeProcessor** — keep the branch's `m_pending_logical_filament_id` reset; drop the old
early `simulate_st_synchronize(extra_time)` (upstream now syncs after `store_move_vertex`).

**`GCodeWriter::_travel_to_z` / `_spiral_travel_to_z`** — these deref `filament()->id()`
for per-extruder travel speed, but `filament()` is null before the first toolchange
(`init_extruder()` runs only for BBL printers before `preamble()`). The branch's non-zero
`get_active_z_offset()` makes `preamble()` do a real silent Z move, so **non-BBL printers
crash** unless `filament()` is null-guarded (fall back to filament 0). Keep this guard
across syncs — upstream has the latent bug (its preamble uses `z_offset.value == 0`).

## Post-merge verification (cheap, do before building)

```bash
# no scalar .value left on now-vector options:
grep -rnE '(default_acceleration|outer_wall_acceleration|inner_wall_acceleration|top_surface_acceleration|initial_layer_acceleration|bridge_acceleration|travel_acceleration|sparse_infill_acceleration|internal_solid_infill_acceleration|bridge_speed|internal_bridge_speed)\.value' src/libslic3r src/slic3r | grep -v travel_short_distance_acceleration
grep -rnE '(default_jerk|outer_wall_jerk|inner_wall_jerk|top_surface_jerk|initial_layer_jerk|travel_jerk|infill_jerk|default_junction_deviation)\.value' src/libslic3r src/slic3r
# both should return nothing.
```

Then build (`./build_release_macos.sh -x -s -a arm64`) and, ideally, slice one model on a
non-BBL printer (exercises the `_travel_to_z` preamble path).
