# Decision Log

This is a lightweight ADR-style log for modernization decisions. Add entries when architecture, tooling, validation, compatibility, workflow, or behavior-preservation choices change.

## 2026-05-30: Repository Is a Legacy macOS Codebase

Status: Observed

- The project is an Xcode-based macOS application written mostly in C with Objective-C bridge and UI code.
- It references unsupported or legacy technologies including Carbon, QuickTime, old resource formats, NIBs, and legacy SDK/deployment settings.
- The current project may not build or run on modern Apple silicon Macs without a legacy toolchain or porting work.

## 2026-05-30: Modernization Target Is Apple Silicon macOS

Status: Accepted

- The target is a modern macOS application that builds and runs on Apple silicon Macs.
- The port should preserve original Ultima III behavior where practical.
- Unsupported legacy dependencies such as Carbon, QuickTime, classic Mac resources, and old SDK assumptions should be replaced over time.

## 2026-05-30: Use Harness Engineering Before Broad Porting

Status: Accepted

- Modernization will start by creating operating documents, boundaries, validation expectations, and task structure.
- Broad rewrites should wait until there is a characterization or harness path for the affected behavior.
- The harness should support both human maintainers and AI coding agents.

## 2026-05-30: Legacy Code and Assets Are the Reference Implementation

Status: Accepted

- `Sources/` remains the behavioral reference until code is characterized, extracted, or intentionally replaced.
- Historical assets in `Resources/`, `Images/`, `Cursors/`, and `English.lproj/` should not be changed without a documented reason.
- Asset licensing and conversion decisions require care because the README identifies non-code asset caveats.

## 2026-05-30: Target Boundary Is App Shell Plus Portable Game Core

Status: Accepted

- The intended target shape is a modern macOS app shell around a portable, testable game core.
- Platform concerns should be isolated behind adapters for input, rendering, audio, persistence, resource loading, and application shell behavior.
- The portable game core should not depend directly on macOS UI, audio, rendering, filesystem, or resource APIs.

## 2026-05-30: Defer Concrete Feature Work

Status: Accepted

- Concrete feature specs are intentionally deferred.
- Early work should focus on the engineering harness, architecture map, decision log, task template, tracking structure, and modernization inventories.
- Feature and bug templates or trackers may exist before any actual feature or bug documents are created.

## 2026-05-30: Add Feature and Bug Tracking Structure

Status: Accepted

- Feature work will be tracked under `docs/features/` using `FEATURE_TEMPLATE.md` and `docs/features/FEATURE_TRACKER.md`.
- Bugs, regressions, build failures, and behavior mismatches will be tracked under `docs/bugs/` using `BUG_TEMPLATE.md` and `docs/bugs/BUG_TRACKER.md`.
- Trackers may remain empty until real modernization work discovers concrete features or bugs.

## 2026-05-30: Use a Plain C Command-Line Characterization Harness

Status: Accepted

- Early behavior characterization will use a small command-line C harness under `harness/`.
- The harness command is `make -C harness test`.
- The harness should avoid Carbon, QuickTime, legacy SDKs, resource files, NIBs, and app startup.
- The first harness may copy narrow deterministic legacy functions into an isolated test executable with source references when compiling the full legacy file would require broad unrelated platform shims.
- Future extraction work should replace copied harness implementations with shared portable game-core code once a clean boundary exists.

## 2026-05-30: Seed Portable Core Under Top-Level Core Directory

Status: Accepted

- The first shared portable game-core modules should live under a new top-level `Core/` directory, outside legacy `Sources/`.
- Use `Core/include/u3_<domain>.h` for public portable headers and `Core/src/u3_<domain>.c` for implementations.
- Portable public C symbols should use a lowercase `u3_` prefix, such as `u3_map_get_heading`; public constants should use `U3_<DOMAIN>_<NAME>`.
- Core headers should not expose Classic Mac types. Byte-sensitive legacy layouts should use portable fixed-width C types, including a core-owned Pascal string byte alias when needed.
- Behavior that previously depended on globals should move toward small caller-owned state structs passed explicitly into core functions.
- Harness-only compatibility shims stay under `harness/`; reusable game behavior belongs under `Core/`.

## 2026-05-31: Portable Core Extraction Rules

Status: Accepted

- Extracted gameplay logic remains plain portable C for now, compatible with the existing command-line harness approach.
- The portable core must not depend on macOS UI, rendering, audio, input, preferences, filesystem, resource, or application lifecycle APIs.
- Core behavior should use explicit caller-owned state passed through public APIs instead of reading or mutating legacy globals directly.
- Core APIs should return values, result structs, events, command buffers, or other testable outputs instead of performing UI, audio, input, file, or rendering side effects.
- Raw legacy numeric values such as tile IDs, monster types, weapon IDs, class IDs, spell flags, and byte-level status values may remain at extraction boundaries during characterization.
- Named constants and stronger domain types may be introduced as behavior becomes well understood, but they should not obscure byte-for-byte legacy compatibility.
- The command-line harness remains the first validation consumer for portable core work until a legacy or modern app integration point is intentionally chosen.

## 2026-05-31: Independent Code Review After Code Changes

Status: Accepted

- After every coding task, run an independent subagent code review before final handoff.
- The review should use a code-review stance and prioritize behavior drift, missing tests, portability leaks, API/state boundary problems, and C correctness issues.
- Review findings are part of the task and should be resolved before handoff, or explicitly documented as accepted residual risk.
- If subagent tooling is unavailable, record that in the handoff.
- Documentation-only changes do not require a subagent review unless the change records a major decision, feature contract, or workflow rule that would benefit from independent review.
- The implementing agent remains responsible for resolving or explicitly documenting review findings before final handoff.

## 2026-06-03: Mandatory Subagent Review After Coding Tasks

Status: Accepted

- The previous review guidance is strengthened from conditional guidance to a mandatory completion step for coding tasks.
- A coding task is not ready for final handoff until an independent subagent code review has run, unless subagent tooling is unavailable.
- The final handoff should mention the review outcome and any unresolved residual risk.

## 2026-05-31: Portable Core Side Effect Modeling

Status: Accepted

- Portable core functions should model observable side effects as return values, result structs, events, command buffers, or explicit state transitions.
- The portable core should not directly render, play audio, show text, wait for input, read preferences, access files, or call application lifecycle APIs.
- Platform adapters are responsible for translating core results into rendering, sound, text, input, persistence, and application behavior.
- During characterization, result structs and events may expose raw legacy values such as tile IDs, headings, message IDs, command keys, dungeon cell values, or status bytes.
- Harness tests should assert core state transitions and returned results or events, not platform-specific effects.

## 2026-05-31: Decision Checkpoint Before Feature Work

Status: Accepted

- Before starting a new feature or milestone, pause to identify decisions that should be made first.
- The checkpoint should surface architecture, boundary, validation, compatibility, workflow, and behavior-preservation choices that could affect the feature.
- Record accepted durable decisions in `DECISION_LOG.md` before implementation.
- If no decision is needed, state that explicitly and proceed with the feature.
- This checkpoint applies to feature and milestone work, not trivial bug fixes or documentation cleanups unless they change project direction.

## 2026-05-31: Characterization Core Is Not Final App API

Status: Accepted

- Early `Core/` functions are characterization surfaces extracted from legacy behavior, not necessarily final application-facing API contracts.
- Characterization APIs may expose raw legacy values and narrow preconditions while behavior is still being mapped.
- Harness coverage documents the currently validated subset of extracted behavior; legacy source remains the reference for uncovered cases unless an intentional behavior change is recorded.
- Future stable core facades or app integration layers may wrap, harden, rename, or replace characterization APIs with safer contracts once their behavior and consumers are better understood.
- Any helper promoted to a stable app-facing API should have explicit precondition documentation, validation coverage, or input hardening.

## 2026-05-31: Autocombat Macro Results Use Legacy Execution Order

Status: Accepted

- `u3_autocombat_macro.commands[0]` is the next command to execute.
- This matches legacy input handling, where `GetKeyMouse` consumes `Macro[0]`.
- This intentionally differs from raw `AddMacro` call order because legacy `AddMacro` pushes each new key to the front of the macro buffer.
- Direct consumers should execute `u3_autocombat_macro.commands` from index `0` through `length - 1`.
- Adapters that enqueue through legacy `AddMacro` must preserve `Macro[0]` as the next command to execute, for example by enqueueing returned commands in reverse order or by writing the legacy macro buffer directly.

## 2026-06-05: Persistence Adapter Uses Modern Native Save With Legacy Import

Status: Accepted

- The first modern persistence adapter should use a canonical modern save document for native reads and writes, not writable classic Resource Manager files.
- The adapter boundary should preserve the legacy save domains as explicit records: active party bytes, roster bytes, current Sosaria map bytes, current Sosaria monster bytes, mutable `MISC` table bytes, and compatibility metadata.
- Legacy `Ultima III Roster` files should be supported first as best-effort import fixtures through the classic resource parser, after real or synthesized mutable roster fixtures exist.
- Exact legacy export is deferred until mutable roster fixture coverage proves record sizes, resource-map behavior, and save timing well enough to avoid corrupt or misleading exports.
- A new-format-only path is rejected for the first adapter because it would discard feasible compatibility with historical saves and weaken behavior comparison.
- The app shell should provide the concrete save-file location or URL. The persistence adapter should not bake in AppKit, sandbox, Preferences-folder, or application-support path APIs.
- Manual saves must preserve the legacy outdoor-Sosaria restriction: save commands write durable state only when the party is in Sosaria (`Party[3] == 0`) after synchronizing party position bytes.
- Auto-save and transition writes must preserve legacy timing around `PutRoster`, `PutParty`, `PutSosaria`, `PushSosaria`, and `PullSosaria`; temporary pushed Sosaria state remains an in-session snapshot unless a legacy path explicitly flushes it.
- Persistence writes should be atomic at the adapter boundary: serialize a complete save document and replace the previous durable document only after the new one is valid.

## 2026-06-05: First Modern App Shell Uses AppKit With SwiftUI Panels

Status: Accepted

- The first modern macOS shell must use an AppKit-owned application lifecycle, primary window, menu/command routing, keyboard and mouse event routing, fullscreen/window behavior, game host view, and concrete save-file location provider.
- SwiftUI must be used only for preferences, inspectors, setup flows, and future non-game panels unless a later decision explicitly changes ownership.
- SwiftUI must not own the shell lifecycle, command model, primary game surface, deterministic game loop, or first-hop game input routing in the first shell milestone.
- `Core/` public headers must not expose AppKit, SwiftUI, SDL, Metal, AVFoundation, Foundation filesystem, sandbox, or other platform framework types.
- Renderer, input, audio, resource, and persistence behavior must remain behind adapters and communicate with the portable core through portable C data, result structs, events, command buffers, or explicit state transitions. The shell decision does not choose the final renderer, final audio implementation, final asset conversion path, or persistence format beyond the F-014 direction.
- The shell may provide concrete bundle and save-file URLs or paths, but resource parsing and persistence serialization must remain adapter-owned.
- SDL3 remains a possible renderer/input portability fallback, especially inside an AppKit-owned shell, but a pure SDL shell is rejected for the first milestone because it weakens native macOS menus, preferences, save-file presentation, accessibility, and app conventions.
- Follow-up work should start with a minimal AppKit shell seed, then separate renderer, input, audio, and resource/persistence adapter milestones.
