# Porting Strategy

Use this document to plan the modernization path from the legacy Mac application to a modern Apple silicon-compatible macOS app.

## Strategy

1. Inventory legacy systems and platform dependencies.
2. Identify logic that can be characterized without the full app.
3. Build small harnesses around bounded behavior.
4. Extract or port portable game logic.
5. Replace platform services with modern adapters.
6. Integrate the modern app shell.
7. Validate behavior through tests and manual gameplay checks.

## Target Boundaries

- Portable game core
- Input adapter
- Renderer adapter
- Audio adapter
- Resource and persistence adapter
- Modern macOS app shell

## Sequencing Notes

Prefer subsystems with clear inputs and outputs before tightly coupled UI/rendering work.

## Risks

- Legacy global state may make extraction difficult.
- Classic Mac types may be mixed into game logic.
- Resource loading and rendering may be tightly coupled.
- Some original behavior may be hard to observe if the app cannot run.

