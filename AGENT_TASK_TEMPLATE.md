# Agent Task Template

Use this template when assigning implementation, inventory, documentation, characterization, or porting work to an AI coding agent in this repository.

## Objective

Describe the concrete outcome. Include whether the task is inventory, characterization, extraction, adapter work, replacement, documentation, or modern app work.

Example: `Inventory Carbon and QuickTime touchpoints in the legacy source tree.`

## Constraints

- Follow `ENGINEERING_HARNESS.md`.
- Preserve legacy source and assets unless the task explicitly requires a change.
- Reuse existing documentation structure before adding new process files.
- Keep changes narrow and tied to the objective.
- Do not assume the legacy app builds on the current machine.
- Do not create feature files unless the task explicitly asks for feature work.

## Affected Systems

Mark the expected areas:

- `Sources/` legacy source
- `Resources/` bundled assets
- `English.lproj/` NIBs and localized metadata
- `Ultima3.xcodeproj/` build configuration
- `docs/modernization/` inventory and strategy docs
- `docs/features/` feature planning
- `docs/bugs/` bug tracking
- root harness docs
- tests or harness code

## Implementation Notes

List relevant files, likely legacy functions, data structures, platform APIs, edge cases, and ambiguity. If behavior is inferred rather than proven, say so.

## Execution Steps

1. Read the relevant harness docs and source files.
2. Identify affected boundaries and legacy behavior.
3. Add or update characterization coverage where practical.
4. Make the smallest coherent change.
5. Run applicable validation.
6. Update docs when architecture, strategy, inventory, or decisions change.

## Validation Steps

Use the strongest practical validation for the task. Examples:

```bash
xcodebuild -project Ultima3.xcodeproj -target Ultima3 -configuration Debug build
```

Add future test or harness commands here once they exist. Record skipped validation with the reason.

## Definition of Done

- Objective is satisfied.
- Existing behavior is preserved unless intentionally changed.
- Relevant documentation is updated.
- Tests, harnesses, or manual checks are added where practical.
- Validation passes, or failures and skipped checks are documented.
- New durable assumptions or decisions are added to `DECISION_LOG.md`.

## Risks

Identify risks such as legacy global state, platform API coupling, historical asset licensing, save compatibility, or behavior that cannot currently be observed by running the app.

## Rollback Considerations

Describe how to revert safely. Note whether changes affect legacy source, assets, generated files, documentation only, or future harness code.
