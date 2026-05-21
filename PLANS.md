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

## Handoff
- Agent: Codex
- Date: 2026-05-21
- Completed this session:
  - Fetched the current `upstream/main` state.
  - Started a no-commit merge of `upstream/main` into `ultimate-merge.v2`.
  - Resolved the two new content conflicts in `src/libslic3r/GCode.cpp` and `src/libslic3r/GCode/GCodeProcessor.cpp`.
  - Staged the resolved conflict files so the merge has no unresolved paths.
- Stopped at:
  - The merge result is staged and ready for review; no merge commit was created.
- Next step:
  - Review the staged merge result, run any desired build/smoke test, and create the merge commit if acceptable.
- Open blockers:
  - none
- Decisions made this session:
  - `GCode.cpp` keeps the branch's unified `append_tcr` wipe-tower path and does not restore upstream's deleted `append_tcr2` implementation.
  - `GCodeProcessor.cpp` keeps branch preheat override handling while using upstream `current_layer_id` tracking for first-layer temperature selection.

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
