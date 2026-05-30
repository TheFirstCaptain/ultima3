# Validation Strategy

Use this document to define how modernization work proves behavior was preserved or intentionally changed.

## Current State

The legacy app may not build or run on modern Apple silicon Macs. There is no automated test suite yet.

## Validation Levels

1. Static inspection of relevant legacy files and project metadata.
2. Legacy build attempt when a suitable toolchain is available.
3. Focused harness tests around extracted or ported behavior.
4. Manual verification for gameplay, rendering, input, audio, and save/load behavior.
5. Modern app build and smoke test once a modern target exists.

## Characterization Targets

| Target | Legacy Reference | Harness/Test Approach | Status |
| --- | --- | --- | --- |
| Map/math accessors | `Sources/UltimaMisc.c`: `GetHeading`, `Absolute`, `GetXY`, `PutXY`, `MapConstrain`, `GetXYVal`, `PutXYVal` | Drive synthetic globals and arrays; assert returned values and state mutations, including wraparound and out-of-bounds map behavior. | Recommended first |
| Pascal string helpers | `Sources/UltimaText.c`: `StringLocation`, `SearchReplace`, `IsNewline`, `AddString` | Use byte-level `Str255` fixtures; assert exact length byte and content bytes after mutation. | Candidate |
| Combat movement predicates | `Sources/UltimaSpellCombat.c`: `CombatValidMove`, `CombatMonsterHere`, `CombatCharacterHere` | Populate combat globals; assert passability return codes and entity lookup priority. Stub sound before including `ValidMove`. | Candidate |
| Autocombat targeting helpers | `Sources/UltimaAutocombat.c`: `ThreatValue`, `MonsterNearby`, `MonsterLinedUp`, `AutoMoveChar` | Populate party/monster/future-position globals; shim no-diagonal preference; assert keypad command outputs. | Candidate |
| Dungeon navigation deltas | `Sources/UltimaDngn.c`: `Forward`, `Retreat`, `Left`, `Right`, `dDescend`, `dKlimb` | Stub rendering/messages; drive dungeon cells and heading globals; assert position, heading, level, and exit state. | Later candidate |

## First Target Rationale

Start with the map/math accessors in `Sources/UltimaMisc.c`. They are deterministic, important to later combat and map movement behavior, and can likely be characterized with synthetic arrays and globals without resource files, QuickDraw, Carbon windows, audio, or a runnable app. They also expose legacy byte-wrap behavior early, especially in `Absolute()` and out-of-bounds map handling.

## F-001D Inspection Notes

- Explorer subagent performed read-only analysis.
- No build or test run was attempted because this was an analysis-only pass.
- Candidate ranking is based on apparent input/output clarity, platform coupling, and harness setup cost.

## Skipped Validation

Record skipped checks with the reason, date, and impact.
