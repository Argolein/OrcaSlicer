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

## Handoff
- Agent: Codex
- Date: 2026-06-02
- Completed this session:
  - Fixed the 1-extruder Orca-managed filament mapping path in `src/slic3r/GUI/PartPlate.cpp` and `src/slic3r/GUI/Plater.cpp` by removing the `> 1 extruder` activation requirement.
  - Updated `src/slic3r/GUI/Tab.cpp` so toggling `Enable Orca-managed extruder mapping` refreshes the filament badges immediately.
  - Kept the filament edit button opening the normal preset editor for managed-mapping printers with only one physical extruder.
  - Verified the result with `git diff --check`.
- Stopped at:
  - The working tree contains the new GUI fix plus the earlier cherry-pick commit on `ultimate-merge.v2`; the changes are not committed yet.
- Next step:
  - Smoke-test the printer settings toggle and filament badges in the UI, then commit if the behavior matches expectations.
- Open blockers:
  - none
- Decisions made this session:
  - 1-extruder managed mapping should normalize all filament badges to `1`, but it should not force the edit button into a mapping menu with no real mapping choices.

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
