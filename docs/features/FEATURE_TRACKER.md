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
| F-007 | Autocombat Macro Decision Core | Complete | Done | [F-007.md](./F-007.md) | 2026-05-31 | Full `AutoCombat` macro decision flow extracted as portable command generation and harness-covered. |
| F-008 | Dungeon Navigation Core | Complete | Done | [F-008.md](./F-008.md) | 2026-05-31 | Bounded dungeon movement, turning, level, and exit state transitions extracted and harness-covered. |
| F-009 | Combat Action Resolution Inventory | Complete | Done | [F-009.md](./F-009.md) | 2026-06-02 | Combat action side effects inventoried; next extraction targets proposed. |
| F-009A | Monster Damage Resolution Core | Complete | Done | [F-009A.md](./F-009A.md) | 2026-06-03 | `DamageMonster` extracted to portable core and harness-covered. |
| F-009B | Player Combat Attack Resolution Core | Complete | Done | [F-009B.md](./F-009B.md) | 2026-06-03 | Post-direction `CombatAttack` extracted to portable core and harness-covered. |
| F-009C | Monster Combat Action Resolution Core | Complete | Done | [F-009C.md](./F-009C.md) | 2026-06-03 | Bounded monster action resolution extracted to portable core and harness-covered. |
| F-010 | Save and Resource Format Inventory | Complete | Done | [F-010.md](./F-010.md) | 2026-06-04 | Save/resource inventory complete; follow-ups proposed for fixtures, resource enumeration, persistence adapter design, and map/talk/combat fixtures. |
| F-011 | Modern App Shell Decision Spike | Complete | Done | [F-011.md](./F-011.md) | 2026-06-05 | Accepted AppKit-owned shell with SwiftUI secondary panels; follow-up shell and adapter milestones proposed. |
| F-011A | Modern App Shell Options Matrix | Complete | Done | [F-011A.md](./F-011A.md) | 2026-06-05 | Tiered shell-options matrix created for F-011 decision input. |
| F-012 | Save Resource Fixture Extractor | Complete | Done | [F-012.md](./F-012.md) | 2026-06-05 | Read-only classic resource parser and harness validation added for default save-template records. |
| F-013 | Resource Data Extraction Inventory | Complete | Done | [F-013.md](./F-013.md) | 2026-06-05 | `MainResources.rsrc` inventory complete; runtime-relevant resource families mapped to consumers and follow-ups. |
| F-014 | Persistence Adapter Design | Complete | Done | [F-014.md](./F-014.md) | 2026-06-05 | Accepted native modern save document with best-effort legacy roster import; exact legacy export deferred. |
| F-015 | Map/Talk/Combat Fixture Characterization | Complete | Done | [F-015.md](./F-015.md) | 2026-06-05 | Representative map, talk, dungeon, and combat screen fixtures characterized in the harness. |
| F-016 | Minimal AppKit/SwiftUI Shell Seed | Complete | Done | [F-016.md](./F-016.md) | 2026-06-05 | SwiftPM modern shell scaffold added with AppKit-owned host, SwiftUI preferences boundary, and C core linkage. |
| F-017 | Renderer Adapter Spike | Complete | Done | [F-017.md](./F-017.md) | 2026-06-05 | Portable render frame contract added with AppKit synthetic tile-grid renderer smoke path. |
| F-018 | Input Adapter Spike | Complete | Done | [F-018.md](./F-018.md) | 2026-06-05 | Portable input event queue added and wired through shell keyboard, mouse, and menu smoke commands. |
| F-019 | Audio Adapter Spike | Complete | Done | [F-019.md](./F-019.md) | 2026-06-05 | Portable audio event queue added with AVFoundation shell adapter and Test Sound smoke command. |
| F-020 | Resource and Persistence Shell Integration | Complete | Done | [F-020.md](./F-020.md) | 2026-06-06 | Shell location, bundle resource, native save fixture, atomic write smoke, and legacy import fixture slices complete. |
| F-020A | Shell Location Provider Smoke | Complete | Done | [F-020A.md](./F-020A.md) | 2026-06-06 | AppKit shell now reports resource-root and save-document candidates in the game host status overlay. |
| F-020B | Bundle Resource Adapter Smoke | Complete | Done | [F-020B.md](./F-020B.md) | 2026-06-06 | Shell resource adapter validates `MainResources.rsrc`; harness covers root-path resource validation. |
| F-020C | Native Save Fixture Builder | Complete | Done | [F-020C.md](./F-020C.md) | 2026-06-06 | Deterministic chunked native new-game save fixture builder added and harness-covered. |
| F-020D | Save Path and Atomic Write Smoke | Complete | Done | [F-020D.md](./F-020D.md) | 2026-06-06 | Shell save smoke command and temp-path atomic write/read harness coverage added. |
| F-020E | Legacy Roster Import Fixture Spike | Complete | Done | [F-020E.md](./F-020E.md) | 2026-06-06 | Synthesized mutable legacy roster resource fixture import added and harness-covered. |
| F-021 | New Game Save Domain Smoke | Complete | Done | [F-021.md](./F-021.md) | 2026-06-07 | Byte-preserving save-domain state loader, harness coverage, and shell debug smoke command complete. |
| F-022 | Resource-Backed Render Smoke | Complete | Done | [F-022.md](./F-022.md) | 2026-06-07 | Resource-backed render frame from `CONS` 400 complete with automated and manual shell validation. |
| F-023 | Shell Game Loop Tick | Complete | Done | [F-023.md](./F-023.md) | 2026-06-07 | Fixed shell tick complete with queued input/audio dispatch, automated coverage, review, and manual validation. |
| F-024 | Party/Roster State Adapter | Complete | Done | [F-024.md](./F-024.md) | 2026-06-07 | Portable decoded roster summary and Game menu status readout complete with automated, review, and manual validation. |
| F-025 | Overworld Movement Smoke | Complete | Done | [F-025.md](./F-025.md) | 2026-06-09 | Bounded modern-shell overworld movement smoke with resource-backed redraw and harness coverage complete. |
| F-026 | Character Creation Flow Spike | Complete | Done | [F-026.md](./F-026.md) | 2026-06-09 | Non-persistent SwiftUI setup panel with portable character candidate validation complete. |
| F-027 | MISC Table Length Policy | Complete | Done | [F-027.md](./F-027.md) | 2026-06-10 | Native save policy preserves 11-byte `MISC` 501..503 records and resolves B-002 with residual legacy-import risk documented. |
| F-028 | Persistent Character Creation | Complete | Done | [F-028.md](./F-028.md) | 2026-06-10 | Create Character persists valid candidates into the first empty native `ROST` slot in the current save document. |
| F-029 | Party Assembly From Roster | Complete | Done | [F-029.md](./F-029.md) | 2026-06-11 | Ordered party assembly from occupied roster slots writes native `PRTY` state and is restored from saved native documents at launch. |
| F-030 | Current Native Save Load and Write | Complete | Done | [F-030.md](./F-030.md) | 2026-06-10 | Shell owns a current native save document, can load new/saved documents, inspect current party state, and atomically write the current document. |
| F-031 | Save-Backed Overworld Movement | Complete | Done | [F-031.md](./F-031.md) | 2026-06-13 | Party-position-backed wrapped movement, centered current Sosaria viewport, and save/reload persistence complete. |
| F-032 | First Overworld Gameplay Interaction Slice | Complete | Done | [F-032.md](./F-032.md) | 2026-06-13 | Non-ship terrain/passability blocks impassable overworld movement and reports target tile results. |
| F-033 | Overworld Turn Economy Slice | Complete | Done | [F-033.md](./F-033.md) | 2026-06-16 | Save-backed handled overworld movement attempts now increment the legacy `PRTY` move counter by active party size. |
| F-034 | Overworld Command Feedback and Event Results | Complete | Done | [F-034.md](./F-034.md) | 2026-06-13 | Portable overworld result fields now drive shell status, redraw, and Step/Bump audio feedback. |
| F-035 | First Overworld Location Transition Slice | Complete | Done | [F-035.md](./F-035.md) | 2026-06-21 | Explicit Enter detects LCB Towne from save-backed location/map data and returns a non-mutating portable transition request. |
| F-036 | Save Workflow and Autosave Policy | Complete | Done | [F-036.md](./F-036.md) | 2026-06-21 | Manual Sosaria-only save workflow, safe atomic writes, dirty-state tracking, launch restore, and quit handling complete. |
| F-037 | Location Session State and Resource Loading | Complete | Done | [F-037.md](./F-037.md) | 2026-06-21 | Validated transient town/castle/dungeon records now load into an adapter-owned session and render a static LCB Towne frame. |
| F-038 | Two-Dimensional Location Navigation | Complete | Done | [F-038.md](./F-038.md) | 2026-06-21 | Town/castle sessions now support non-wrapping movement, centered redraws, feedback, turn accounting, and pending exits. |
| F-039 | First Town Entry and Sosaria Return Round Trip | Complete | Done | [F-039.md](./F-039.md) | 2026-06-22 | Transactional town entry, navigation, save restriction, and exact Sosaria restoration loop complete. |
| F-040 | First Town NPC Talk Slice | Complete | Done | [F-040.md](./F-040.md) | 2026-06-22 | Directional adjacent-NPC lookup and bounded bundled town dialogue display complete. |
| F-041 | Dungeon Entry and Session Integration | Complete | Done | [F-041.md](./F-041.md) | 2026-06-22 | Dungeon Doom entry now initializes a validated transient dungeon session with retained Sosaria context. |
| F-042 | Portable Dungeon Perspective Render Contract | Complete | Done | [F-042.md](./F-042.md) | 2026-06-30 | Renderer-neutral dungeon perspective frame now displays validated Dungeon Doom sessions through the AppKit adapter. |
| F-043 | Dungeon Navigation and Surface Return | Complete | Done | [F-043.md](./F-043.md) | 2026-07-11 | Dungeon commands are wired through the tick path, redraw perspective frames, and restore retained Sosaria on exit. |
| F-044 | Dungeon Light and Torch State | Complete | Done | [F-044.md](./F-044.md) | 2026-07-11 | Dungeon sessions now start dark, Ignite consumes roster torch inventory, light decays on handled dungeon commands, and torch changes persist after return/save. |
| F-045 | Dungeon Passive Turn Mechanics and Encounter Requests | Complete | Done | [F-045.md](./F-045.md) | 2026-07-11 | Post-command dungeon turns now run portable passive mechanics, Pass is handled, light decay is integrated, and empty-floor encounter requests report deterministic monster/combat-screen metadata without starting combat. |
| F-046 | Dungeon Special Tile Effects Slice | Complete | Done | [F-046.md](./F-046.md) | 2026-07-12 | Wind, traps, gremlin food theft, and writing now run through portable results with shell status/audio/redraw and bounded save/session mutations. |
| F-047 | Dungeon Interactive Tile Flows | Proposed | Proposed | [F-047.md](./F-047.md) | 2026-07-09 | Add bounded prompt-backed dungeon interactions for fountains, marks, message fixtures, and the first chest path. |
