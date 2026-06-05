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

## Persistence Direction

F-014 accepts a first persistence boundary: native modern saves should use a canonical modern save document, while legacy `Ultima III Roster` files should be supported first as best-effort imports after mutable roster fixtures exist. Exact legacy export is deferred.

The persistence adapter should own byte-preserving records for active party, roster, current Sosaria map, current Sosaria monsters, and mutable `MISC` tables. The app shell should provide the concrete storage location; the adapter should not directly depend on AppKit, Carbon, Preferences-folder, sandbox, or application-support APIs.

## Sequencing Notes

Prefer subsystems with clear inputs and outputs before tightly coupled UI/rendering work.

The first recommended characterization target is the map/math accessor slice in `Sources/UltimaMisc.c`: `GetHeading`, `Absolute`, `GetXY`, `PutXY`, `MapConstrain`, `GetXYVal`, and `PutXYVal`. This slice appears deterministic and mostly independent of Carbon, QuickDraw, QuickTime, resource files, and app startup.

Likely early sequence:

1. Create a small harness/test structure for legacy C behavior.
2. Characterize map/math accessors with synthetic globals and arrays.
3. Characterize byte-level Pascal string helpers.
4. Characterize combat movement predicates.
5. Seed shared portable modules under `Core/include/u3_<domain>.h` and `Core/src/u3_<domain>.c`.
6. Update harness tests to compile those modules directly instead of local copied behavior.

Initial extraction guidance:

- Keep `Core/` outside legacy `Sources/` until integration with the app target is intentional.
- Use lowercase `u3_` names for public C APIs and `U3_<DOMAIN>_<NAME>` for public constants.
- Prefer explicit caller-owned state structs for behavior that currently reads or mutates globals.
- Keep harness-only shims in `harness/`; move only reusable game behavior into `Core/`.

## Risks

- Legacy global state may make extraction difficult.
- Classic Mac types may be mixed into game logic.
- Resource loading and rendering may be tightly coupled.
- Some original behavior may be hard to observe if the app cannot run.
- Early harness work may require shims for Mac types, handles, Pascal strings, or globals before any useful test can compile.
