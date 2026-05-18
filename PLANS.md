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

## Handoff
- Agent: Codex
- Date: 2026-05-17
- Completed this session:
  - Fetched current remotes.
  - Merged `upstream/main` into `ultimate-merge.v2`.
  - Resolved all eight content conflicts with local branch behavior prioritized.
  - Staged the resolved conflict files so the merge has no unresolved paths.
  - Fixed follow-up macOS build errors in `Print.hpp`, `TreeSupport.cpp`, and `SnapmakerPrinterAgent.cpp`.
  - Verified `./build_release_macos.sh -x -s -a arm64` completes successfully.
- Stopped at:
  - Merge/build fixes are staged, but no commit was created.
- Next step:
  - Review the staged changes and create the merge commit if the result is acceptable.
- Open blockers:
  - none
- Decisions made this session:
  - Local branch changes take priority in conflict resolution.
  - `Preset.cpp` uses the current upstream option-list layout with local branch options re-added.
  - `GCode.cpp` keeps local short-travel acceleration behavior and integrates upstream first-layer travel acceleration/jerk.
  - `SnapmakerPrinterAgent.cpp` keeps the local stable vendor/type fallback after upstream color-aware matching.
  - `TreeSupport.cpp` keeps the local bottom-gap cleanup by deriving `bottom_gap_layers` from upstream's `bottom_gap_height`.

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
