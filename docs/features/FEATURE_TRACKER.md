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
| F-001 | Modernization Analysis Baseline | Complete | Done | [F-001.md](./F-001.md) | 2026-05-30 | Initial legacy inventory and characterization target analysis complete. |
| F-001A | Source Responsibility Inventory | Complete | Done | [F-001A.md](./F-001A.md) | 2026-05-30 | Initial source responsibility inventory added to modernization docs. |
| F-001B | Platform Dependency Inventory | Complete | Done | [F-001B.md](./F-001B.md) | 2026-05-30 | Platform dependency inventory added to modernization docs. |
| F-001C | State and Coupling Inventory | Complete | Done | [F-001C.md](./F-001C.md) | 2026-05-30 | State and coupling inventory added to modernization docs. |
| F-001D | Characterization Candidate Analysis | Complete | Done | [F-001D.md](./F-001D.md) | 2026-05-30 | Recommended first target: map/math accessors in `UltimaMisc.c`. |
| F-002 | First Porting Harness | Proposed | Proposed | [F-002.md](./F-002.md) | 2026-05-30 | Umbrella milestone for the first executable characterization path. |
| F-002A | Harness Tooling Decision | Proposed | Proposed | [F-002A.md](./F-002A.md) | 2026-05-30 | Decide the minimal test/harness approach before implementation. |
| F-002B | Map/Math Accessor Characterization | Proposed | Proposed | [F-002B.md](./F-002B.md) | 2026-05-30 | First recommended characterization target from F-001D. |
| F-002C | Pascal String Helper Characterization | Proposed | Proposed | [F-002C.md](./F-002C.md) | 2026-05-30 | Candidate second characterization target. |
| F-002D | Combat Predicate Characterization | Proposed | Proposed | [F-002D.md](./F-002D.md) | 2026-05-30 | Candidate third characterization target. |
