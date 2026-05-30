# Feature Tracker

Use this tracker to preserve feature state across sessions and agent handoffs. Do not add implementation details here; link to a feature document when one exists.

## Phases

- `Proposed`: Feature exists as a plain-language outcome.
- `Clarifying`: Requirements, constraints, and open questions are being gathered.
- `Planned`: Implementation approach is documented for review.
- `Approved`: Plan is approved for implementation.
- `Building`: Implementation is in progress.
- `Validating`: Tests, build checks, or manual verification are in progress.
- `Reviewing`: Review findings are being gathered or addressed.
- `Done`: Feature is complete and validated.
- `Blocked`: Work cannot continue without input or an external change.
- `Deferred`: Feature is intentionally paused.

## Tracking Conventions

- Use an umbrella feature for a durable modernization milestone with multiple related workstreams.
- Use child features, such as `F-001A`, only when the work can be delegated, validated, or completed independently.
- Keep small implementation steps inside the feature document instead of adding tracker rows.
- Prefer fewer, outcome-oriented tracker entries over a task list of every action.
- Roll child findings back into the umbrella feature before marking the umbrella `Done`.

## Tracker

| ID | Title | Status | Phase | Feature Doc | Last Updated | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| F-001 | Modernization Analysis Baseline | Proposed | Proposed | [F-001.md](./F-001.md) | 2026-05-30 | Umbrella analysis milestone for the first legacy inventory pass. |
| F-001A | Source Responsibility Inventory | Proposed | Proposed | [F-001A.md](./F-001A.md) | 2026-05-30 | Candidate subagent task. |
| F-001B | Platform Dependency Inventory | Proposed | Proposed | [F-001B.md](./F-001B.md) | 2026-05-30 | Candidate subagent task. |
| F-001C | State and Coupling Inventory | Proposed | Proposed | [F-001C.md](./F-001C.md) | 2026-05-30 | Candidate subagent task. |
| F-001D | Characterization Candidate Analysis | Proposed | Proposed | [F-001D.md](./F-001D.md) | 2026-05-30 | Candidate subagent task. |
