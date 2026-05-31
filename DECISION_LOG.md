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
