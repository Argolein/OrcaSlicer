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

**When upstream adds a new dependency** (2026-07-30 sync added `python3`, `pybind11`,
`wxInspector` for the Python plugin system), `-s` is not enough — it builds the slicer only.
Build deps first, and **without `-x`**: the existing `deps/build/arm64` cache was configured
with `Unix Makefiles`, and `-x` switches deps to Ninja, which fails with
`generator ... does not match the generator used previously`.

```bash
./build_release_macos.sh -d -a arm64           # deps, default (Unix Makefiles) generator
./build_release_macos.sh -x -s -a arm64        # then the slicer — no -b, the cache must regenerate
```

(The usual day-to-day command is `-x -s -a arm64 -b`; `-b` skips the CMake reconfigure, so drop it
after a merge that adds or renames source files.)

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
| Tower interface preheat | GCode.cpp `use_support_tower_interface_temp()` | narrower than upstream's inline `enable_tower_interface_features && tcr.is_contact` — also requires `filament_is_support` |
| Wipe-tower purge model | Print.cpp `_make_wipe_tower()` | branch splits prime `wipe_volume` vs `purge_volume` + `purge_in_prime_tower && SEMM` path w/ `filament_minimal_purge_on_wipe_tower` add-back |

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

**Mapped tool ids vs raw filament ids (recurs in ~10 GCode.cpp hunks).** Upstream keeps
rewriting the placeholder blocks and always writes the *raw filament id*. The branch must keep
`get_toolchange_id(...)` for the tool-facing placeholders — `previous_extruder`, `next_extruder`,
`current_extruder`, `filament_extruder_id`, `initial_no_support_tool/_extruder` — and for the
`custom_gcode_changes_tool(...)` comparison. Everything else upstream adds in those blocks
(`current_filament_id`, `current_nozzle_id`, `*_hotend`, `*_extruder_variant`,
`nozzle_diameter_at_nozzle_id`, `nozzle_volume_types`) is additive: take it.

**Per-variant config columns.** Upstream added `get_filament_config_index(fid)` /
`get_nozzle_config_index(fid)`; `NOZZLE_CONFIG(x)` is now
`m_config.x.get_at(get_nozzle_config_index(m_writer.filament()->id()))`. Filament-indexed reads
(`retraction_distances_when_cut`, `nozzle_temperature`, …) go through `get_filament_config_index`,
and `get_abs_value_at(...)` accel/jerk reads through `get_nozzle_config_index` — **not**
`cur_extruder_index()`, which is the plain extruder index and mismatches the macro on
multi-variant printers.

**`GCodeWriter::toolchange`** is 2-arg (`filament_id, nozzle_id`); the 1-arg form is gone. Keep the
branch's `;VT` emission and the `physical_tool_changed` gate around the T/M1020 emission (a logical
filament switch on the same physical extruder must stay metadata-only), and fold upstream's
`!config.manual_filament_change` guard and ` H<nozzle_id>` suffix inside it.

**`Print::update_filament_maps_to_config`** is now 3-arg (`f_maps, f_volume_maps, f_nozzle_maps`,
the latter two defaulted). Keep the branch's `normalize_filament_maps(...)` call at the top.

**`PresetBundle::full_config`** takes an optional third `filament_volume_maps`. The branch's
`full_config_for_current_mapping_mode()` helpers (PartPlate.cpp, Plater.cpp) own that composition —
update the helper, not the ~5 call sites, which upstream keeps re-inlining.

**GCodeProcessor** — keep the branch's `m_pending_logical_filament_id` reset; drop the old
early `simulate_st_synchronize(extra_time)` (upstream now syncs after `store_move_vertex`).

**`GCodeWriter::_travel_to_z` / `_spiral_travel_to_z`** — ~~these deref `filament()->id()`~~
**Resolved upstream (2026-07-30 sync).** Upstream now caches `m_cached_extruder_idx`
(ctor/`reset()` default 0, refreshed in `toolchange()`) and the travel-speed lookups read that
instead of dereferencing `filament()`. The default of 0 is exactly the branch's old fallback, so
the branch's null-guard was dropped as redundant. If a future sync reintroduces a `filament()`
deref on this path, restore the guard — the crash it prevented (non-BBL printers, silent Z move
from `preamble()` because `get_active_z_offset()` is non-zero) is still reachable.

**`;VT` marker spelling.** The branch emits `;VT T<fid>` (GCodeWriter::toolchange,
`virtual_toolchange_comment()`); upstream's newer code emits `;VT<fid>` optionally followed by
` H<nozzle>`. `GCodeProcessor::process_VT()` already accepts both. Upstream's **new** pre-heating
block builder (`handle_filament_change` lambda in the post-process scan) did `str >> fid`
straight after `;VT` and silently no-ops on the branch's spelling — it was patched to skip an
optional `T`. Re-check that parser on every sync rather than changing the branch's marker format
(the format is load-bearing for saved G-code and the strict-physical-tool-id path).

## Post-merge verification (cheap, do before building)

```bash
# no scalar .value left on now-vector options:
grep -rnE '(default_acceleration|outer_wall_acceleration|inner_wall_acceleration|top_surface_acceleration|initial_layer_acceleration|bridge_acceleration|travel_acceleration|sparse_infill_acceleration|internal_solid_infill_acceleration|bridge_speed|internal_bridge_speed)\.value' src/libslic3r src/slic3r | grep -v travel_short_distance_acceleration
grep -rnE '(default_jerk|outer_wall_jerk|inner_wall_jerk|top_surface_jerk|initial_layer_jerk|travel_jerk|infill_jerk|default_junction_deviation)\.value' src/libslic3r src/slic3r
# both should return nothing.
```

Then build (`./build_release_macos.sh -x -s -a arm64`) and, ideally, slice one model on a
non-BBL printer (exercises the `_travel_to_z` preamble path).
