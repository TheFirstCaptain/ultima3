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
| F-002 | First Porting Harness | Complete | Done | [F-002.md](./F-002.md) | 2026-05-30 | First executable characterization harness complete with map/math, Pascal string, and combat predicate coverage. |
| F-002A | Harness Tooling Decision | Complete | Done | [F-002A.md](./F-002A.md) | 2026-05-30 | Plain C command-line harness accepted; command is `make -C harness test`. |
| F-002B | Map/Math Accessor Characterization | Complete | Done | [F-002B.md](./F-002B.md) | 2026-05-30 | Initial map/math accessor characterization added and passing. |
| F-002C | Pascal String Helper Characterization | Complete | Done | [F-002C.md](./F-002C.md) | 2026-05-30 | Pascal string helper characterization added and passing. |
| F-002D | Combat Predicate Characterization | Complete | Done | [F-002D.md](./F-002D.md) | 2026-05-30 | Combat predicate characterization added and passing. |
| F-003 | Portable Core Seed | Complete | Done | [F-003.md](./F-003.md) | 2026-05-30 | First portable core modules added under `Core/` and used by the harness. |
| F-003A | Portable Core Layout Decision | Complete | Done | [F-003A.md](./F-003A.md) | 2026-05-30 | Accepted top-level `Core/` with `u3_` plain C APIs and explicit caller-owned state guidance. |
| F-003B | Extract Map/Math Accessors | Complete | Done | [F-003B.md](./F-003B.md) | 2026-05-30 | Map/math behavior moved to `Core/` and harness-integrated. |
| F-003C | Extract Pascal String Helpers | Complete | Done | [F-003C.md](./F-003C.md) | 2026-05-30 | Pascal string helper behavior moved to `Core/` and harness-integrated. |
| F-003D | Extract Combat Predicates | Complete | Done | [F-003D.md](./F-003D.md) | 2026-05-30 | Combat predicate behavior moved to `Core/` and harness-integrated. |
| F-004 | Autocombat Targeting Core | Complete | Done | [F-004.md](./F-004.md) | 2026-05-30 | Autocombat targeting helpers moved to `Core/` and harness-integrated. |
| F-005 | Autocombat Movement Forecast Core | Complete | Done | [F-005.md](./F-005.md) | 2026-05-30 | Autocombat forecast helpers added to `u3_autocombat` and harness-covered. |
| F-006 | Autocombat Danger Core | Complete | Done | [F-006.md](./F-006.md) | 2026-05-30 | `MonsterCanAttack` and `NearlyDead` added to `u3_autocombat` and harness-covered. |
| F-007 | Autocombat Macro Decision Core | Proposed | Proposed | [F-007.md](./F-007.md) | 2026-05-30 | Next likely extraction: characterize full `AutoCombat` macro decision flow as portable command generation. |
| F-008 | Dungeon Navigation Core | Proposed | Proposed | [F-008.md](./F-008.md) | 2026-05-31 | Characterize and extract bounded dungeon movement, turning, level, and exit state transitions. |
| F-009 | Combat Action Resolution Inventory | Proposed | Proposed | [F-009.md](./F-009.md) | 2026-05-31 | Map combat action side effects before extracting larger combat resolution behavior. |
| F-010 | Save and Resource Format Inventory | Proposed | Proposed | [F-010.md](./F-010.md) | 2026-05-31 | Inventory mutable save/resource data and compatibility constraints before format decisions. |
| F-011 | Modern App Shell Decision Spike | Proposed | Proposed | [F-011.md](./F-011.md) | 2026-05-31 | Compare modern macOS shell options and record an accepted or deferred direction. |
