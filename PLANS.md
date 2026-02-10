# PLANS.md

## Objective
Move Snapmaker-specific AMS filament matching logic out of `PresetBundle` into `SnapmakerPrinterAgent`, while preserving current matching behavior and avoiding regressions for non-Snapmaker agents.

## Open questions
- (empty)

## Approved plan
- [x] Add Snapmaker-local filament ID resolver (vendor/type/subtype aware, U1/Dual tie-break) in `src/slic3r/Utils/SnapmakerPrinterAgent.*`.
- [x] Use that resolver to populate `tray_info_idx` in Snapmaker AMS payload, with safe fallback to previous generic type mapping.
- [x] Remove Snapmaker-specific matching branches from `src/libslic3r/PresetBundle.cpp` so sync path remains generic.
- [x] Keep behavior parity by preserving existing generic fallbacks in `PresetBundle::sync_ams_list`.
- [x] Run static validation on touched code paths and summarize residual risks.

## Implementation status
- [ ] Not started
- [ ] In progress
- [x] Done

## Decisions
- Keep non-Snapmaker behavior unchanged.
- Prefer behavior-preserving migration: copy matching heuristic to agent first, then simplify `PresetBundle`.
- No additional build/smoke run in this step per user request; validation is static review of changed code paths.

## Notes
- If you plan to do a smoke test, ask before starting to build the project (e.g. for c++ projects)
- Add template to existing PLANS.md and keep it updated during the porject
