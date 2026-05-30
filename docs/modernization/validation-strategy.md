# Validation Strategy

Use this document to define how modernization work proves behavior was preserved or intentionally changed.

## Current State

The legacy app may not build or run on modern Apple silicon Macs. A command-line harness now exists for bounded C behavior that can be tested without the full app. The first shared portable core modules live under `Core/` and are compiled by the harness.

## Harness Commands

```bash
make -C harness test
```

## Validation Levels

1. Static inspection of relevant legacy files and project metadata.
2. Legacy build attempt when a suitable toolchain is available.
3. Focused harness tests around extracted or ported behavior.
4. Manual verification for gameplay, rendering, input, audio, and save/load behavior.
5. Modern app build and smoke test once a modern target exists.

## Characterization Targets

| Target | Legacy Reference | Harness/Test Approach | Status |
| --- | --- | --- | --- |
| Map/math accessors | `Sources/UltimaMisc.c`: `GetHeading`, `Absolute`, `GetXY`, `PutXY`, `MapConstrain`, `GetXYVal`, `PutXYVal` | `harness/map_math_accessors_tests.c` compiles `Core/src/u3_map_math.c`; drives synthetic state and asserts returned values and state mutations, including wraparound and out-of-bounds map behavior. | Harness passing; extracted to `Core/` |
| Pascal string helpers | `Sources/UltimaText.c`: `StringLocation`, `SearchReplace`, `IsNewline`, `AddString` | `harness/pascal_string_helpers_tests.c` compiles `Core/src/u3_pascal_string.c`; uses byte-level fixtures and asserts exact length byte, content bytes, terminal-match quirks, replacement mutation, newline detection, append behavior, and length-byte wrapping. | Harness passing; extracted to `Core/` |
| Combat movement predicates | `Sources/UltimaSpellCombat.c`: `CombatValidMove`, `CombatMonsterHere`, `CombatCharacterHere` | `harness/combat_predicates_tests.c` compiles `Core/src/u3_combat.c`; populates combat state and asserts passability return codes, monster lookup priority, character lookup priority, and inactive-coordinate behavior. `ValidMove` remains deferred because it has audio side effects. | Harness passing; extracted to `Core/` |
| Autocombat targeting and forecast helpers | `Sources/UltimaAutocombat.c`: `ThreatValue`, `MonsterNearby`, `MonsterLinedUp`, `AutoMoveChar`, `SetupNow`, `SetupFuture`, `FutureMonsterHere`, `DirToNearestMonster`, `LineUpToMonster` | `harness/autocombat_targeting_tests.c` compiles `Core/src/u3_autocombat.c`; populates combat state, future monster positions, experience values, tile passability, alive-character flags, and explicit diagonal policy; asserts threat scoring, forecast movement, collision fallback, and keypad command outputs. | Harness passing; extracted to `Core/` |
| Dungeon navigation deltas | `Sources/UltimaDngn.c`: `Forward`, `Retreat`, `Left`, `Right`, `dDescend`, `dKlimb` | Stub rendering/messages; drive dungeon cells and heading globals; assert position, heading, level, and exit state. | Later candidate |

## First Target Rationale

Start with the map/math accessors in `Sources/UltimaMisc.c`. They are deterministic, important to later combat and map movement behavior, and can likely be characterized with synthetic arrays and globals without resource files, QuickDraw, Carbon windows, audio, or a runnable app. They also expose legacy byte-wrap behavior early, especially in `Absolute()` and out-of-bounds map handling.

## F-001D Inspection Notes

- Explorer subagent performed read-only analysis.
- No build or test run was attempted because this was an analysis-only pass.
- Candidate ranking is based on apparent input/output clarity, platform coupling, and harness setup cost.

## Skipped Validation

Record skipped checks with the reason, date, and impact.

- 2026-05-30: Legacy Xcode build not attempted for the first harness step. The affected validation target is isolated C behavior and does not require the legacy app target.
