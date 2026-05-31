# Engineering Harness

This document is the operating guide for modernizing this repository. The goal is to preserve observable Ultima III behavior while replacing legacy Mac platform dependencies in small, verifiable steps.

## Current Reality

- The application is a legacy macOS/Xcode project written mostly in C with Objective-C bridge and UI code.
- It depends on Carbon, QuickTime, old resource formats, NIBs, and SDK settings such as `macosx10.6` and PowerPC deployment paths.
- It may not build or run on a modern Apple silicon Mac without a legacy toolchain or substantial porting work.
- There is no automated test suite yet.
- The README documents licensing caveats for historical non-code assets.

## Modernization Principles

- Characterize behavior before rewriting it.
- Separate portable game logic from platform concerns such as rendering, input, audio, windows, menus, files, and resource loading.
- Keep changes narrow, reversible, and tied to a named porting objective.
- Preserve original behavior unless an intentional change is documented.
- Treat legacy source and assets as the reference implementation until a replacement is proven.
- Record major architecture, tooling, compatibility, and behavior decisions in a decision log once one exists.

## Working Boundaries

- `Sources/` is the legacy source of truth. Avoid broad formatting or mechanical churn in these files.
- `Resources/`, `Images/`, `Cursors/`, and `English.lproj/` are historical asset areas. Modify them only for a documented reason.
- New harness or porting code should live in clearly named directories rather than being mixed casually into legacy files.
- Replacement platform layers should remain isolated by concern: rendering, input, audio, persistence, resource loading, and application shell.
- When extracting logic from legacy code, keep references to the original file/function names in comments or tests where useful.

## Project Documentation

- `ARCHITECTURE.md`: observed legacy architecture and target modernization boundaries.
- `DECISION_LOG.md`: durable architecture, tooling, workflow, compatibility, and behavior decisions.
- `AGENT_TASK_TEMPLATE.md`: standard shape for future agent tasks.
- `docs/features/`: feature templates, feature tracker, and feature-specific planning docs.
- `docs/bugs/`: bug templates, bug tracker, and defect or blocker docs.
- `docs/modernization/`: legacy inventory, porting strategy, and validation strategy.

## Validation Loop

Early modernization work may not have a runnable app. Use the strongest practical validation for the task:

```bash
xcodebuild -project Ultima3.xcodeproj -target Ultima3 -configuration Debug build
```

If the legacy build is not available, document the failure or why it was skipped. As harnesses appear, add focused automated tests for extracted behavior and run the relevant test command before handoff. For UI, rendering, audio, or gameplay changes, include manual verification notes.

## Development Workflow

1. Read the relevant legacy files, project metadata, README, and harness docs before editing.
2. Identify the affected behavior and boundary: inventory, characterization, extraction, adapter, replacement, or modern UI work.
3. Prefer characterization tests or small harness executables before changing logic.
4. Implement the smallest coherent change.
5. Run applicable validation and record skipped checks.
6. Update feature, bug, modernization, architecture, or decision documentation when the work changes status, boundaries, or strategy.
7. For code changes, and for documentation-only changes that record major decisions, feature contracts, or workflow rules, run an independent subagent review before handoff when subagent tooling is available.

## Initial Inventory Targets

- Application startup, main loop, and event handling.
- Global game state and save/load behavior.
- Map, dungeon, combat, spell, and text systems.
- Graphics, tile, cursor, and drawing pipeline.
- String, resource, NIB, sound, and music loading.
- Carbon, QuickTime, and Cocoa bridge responsibilities.

## Definition of Done

- Scope and affected legacy behavior are clear.
- Behavior preservation or intentional behavior change is documented.
- Relevant harness tests, fixtures, or manual checks are added where practical.
- Validation has passed, or failures and skipped checks are explicitly recorded.
- Historical assets are not changed without a documented reason.
- New assumptions or durable decisions are captured in project documentation.

## Agent Guidance

- Inspect before editing.
- Do not assume the legacy app builds.
- Do not rewrite broad systems without first creating a harness or characterization path.
- Prefer adapters and isolated replacement layers over direct platform rewrites.
- Preserve user work and avoid destructive git or filesystem commands.
- State uncertainty explicitly when the code does not prove behavior.
- After code changes, or major decision/contract/workflow documentation changes, use an independent subagent review as a second set of eyes before final handoff when tooling is available.
