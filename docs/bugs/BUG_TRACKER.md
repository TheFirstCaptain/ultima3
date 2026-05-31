# Bug Tracker

Use this tracker for known defects, regressions, build failures, compatibility issues, and modernization blockers.

## Statuses

- `Open`: Confirmed or suspected issue that has not been fixed.
- `Investigating`: Reproduction, scope, or cause is being researched.
- `Fixing`: A fix is in progress.
- `Validating`: The fix is being tested.
- `Resolved`: Fix is complete and validated.
- `Deferred`: Issue is accepted for later work.
- `Won't Fix`: Issue is intentionally not being addressed.

## Priorities

- `P0`: Blocks all useful modernization work.
- `P1`: Blocks a major workflow or subsystem.
- `P2`: Important but does not block broad progress.
- `P3`: Minor, cosmetic, or documentation-only.

## Tracking Conventions

- Use one bug entry for one reproducible defect, build failure, compatibility issue, or blocker.
- Use a blocker entry when the issue affects multiple future features and cannot be cleanly assigned to one feature.
- Keep investigation notes in linked bug documents instead of expanding the tracker table.
- Link bugs to affected feature docs when a defect blocks or is found during feature work.
- Prefer `Deferred` over deleting known issues that are intentionally postponed.

## Tracker

| ID | Title | Status | Priority | Bug Doc | Last Updated | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| B-001 | Pascal String Search/Replace Uses Non-Portable Char Arithmetic | Open | P2 | [B-001.md](./B-001.md) | 2026-05-31 | Found during independent review of F-001 through F-006 portable core code. |
