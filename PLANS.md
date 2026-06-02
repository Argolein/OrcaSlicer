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

## Handoff
- Agent: Codex
- Date: 2026-06-02
- Completed this session:
  - Cherry-picked `691615f6d9` (`multi material mapping`) onto `ultimate-merge.v2`.
  - Resolved cherry-pick conflicts in `src/libslic3r/GCode.cpp`, `src/libslic3r/GCodeWriter.cpp`, `src/slic3r/GUI/GUI_Factories.cpp`, `src/slic3r/GUI/Plater.cpp`, and `src/slic3r/GUI/Tab.cpp`.
  - Verified the resolved tree with `git diff --check`.
- Stopped at:
  - `ultimate-merge.v2` is one commit ahead of `origin/ultimate-merge.v2` with commit `ff9a5dd81c`.
- Next step:
  - Review the cherry-picked result and run any desired build or smoke test before pushing.
- Open blockers:
  - none
- Decisions made this session:
  - Commit `28456c6c3f` was skipped because the cherry-pick was empty on `ultimate-merge.v2`.
  - Tool-related placeholders now use mapped output tool IDs while existing hotend placeholders remain available for custom G-code compatibility.

## Notes
- If a smoke test / build step is needed, ask before starting for expensive C++ builds.
- Keep this file updated throughout the project.
