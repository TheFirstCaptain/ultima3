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
| Monster damage resolution | `Sources/UltimaSpellCombat.c`: `DamageMonster` | `harness/combat_damage_tests.c` compiles `Core/src/u3_combat.c`; populates monster HP, monster-under tiles, combat tile data, monster type, and experience lookup values; asserts HP subtraction, defeat threshold, Lord British immunity, tile restoration, redraw flags, message ID, character number, and experience award event data. | Harness passing; extracted to `Core/` |
| Player combat attack resolution | `Sources/UltimaSpellCombat.c`: post-direction `CombatAttack` | `harness/combat_attack_tests.c` compiles `Core/src/u3_combat.c`; supplies explicit character, direction, weapon, dagger quantity, projectile target, Exodus Castle result, hit rolls, and damage roll; asserts cancel, miss, melee, projectile, thrown dagger mutation, Exodus restriction, hit chance, damage formula, hit side-effect flags, tile state, and `DamageMonster` handoff. | Harness passing; extracted to `Core/` |
| Monster combat action resolution | `Sources/UltimaSpellCombat.c`: monster-turn flow after `FigureNewMonPosition`, `Poison`, and `Pilfer` | `harness/combat_monster_action_tests.c` compiles `Core/src/u3_combat.c`; supplies explicit monster, party size, target, movement intent, projectile hit result, magic target, random rolls, monster HP seed, Exodus flags, equipment state, HP, and experience; asserts movement, archer shooting, magic fallback, poison, pilfer, miss, auto-hit, damage, death, tile restoration, and event flags. | Harness passing; extracted to `Core/` |
| Autocombat targeting, danger, and forecast helpers | `Sources/UltimaAutocombat.c`: `ThreatValue`, `MonsterCanAttack`, `NearlyDead`, `MonsterNearby`, `MonsterLinedUp`, `AutoMoveChar`, `SetupNow`, `SetupFuture`, `FutureMonsterHere`, `DirToNearestMonster`, `LineUpToMonster` | `harness/autocombat_targeting_tests.c` compiles `Core/src/u3_autocombat.c`; populates combat state, future monster positions, experience values, character HP/alive flags, tile passability, and explicit diagonal policy; asserts threat scoring, attack danger, nearly-dead checks, forecast movement, collision fallback, and keypad command outputs. | Harness passing; extracted to `Core/` |
| Autocombat macro decision flow | `Sources/UltimaAutocombat.c`: `AutoCombat` | `harness/autocombat_targeting_tests.c` compiles `Core/src/u3_autocombat.c`; populates explicit class, magic, weapon, spell-cast flags, HP/alive state, monster threat, positions, and diagonal policy; asserts bounded macro command sequences for flee, spell, projectile, lineup, melee, and pass branches. | Harness passing; extracted to `Core/` |
| Dungeon navigation deltas | `Sources/UltimaDngn.c`: `Forward`, `Retreat`, `Left`, `Right`, `dDescend`, `dKlimb` | `harness/dungeon_navigation_tests.c` compiles `Core/src/u3_dungeon.c`; drives synthetic dungeon cells, position, heading, level, and exit state; asserts movement wrapping, blocking, turning, ladder validation, redraw flags, invalid/no-go flags, and dungeon exit behavior. | Harness passing; extracted to `Core/` |
| Save/resource fixture extraction and inventory | `Sources/UltimaMisc.c`: `OpenRstr`, `GetParty`, `GetRoster`, `LoadUltimaMap`, `PutSosaria`, mutable `MISC` table helpers, and other `MainResources.rsrc` consumers | `harness/resource_fixture_tests.c` compiles `Core/src/u3_resource.c`; parses `Resources/English.lproj/MainResources.rsrc` as a read-only classic data-fork resource file, asserts default save-template records, and validates archive inventory totals for runtime-relevant resource families. | Harness passing; default templates and resource inventory characterized; changed user roster fixtures remain future work |

## First Target Rationale

Start with the map/math accessors in `Sources/UltimaMisc.c`. They are deterministic, important to later combat and map movement behavior, and can likely be characterized with synthetic arrays and globals without resource files, QuickDraw, Carbon windows, audio, or a runnable app. They also expose legacy byte-wrap behavior early, especially in `Absolute()` and out-of-bounds map handling.

## F-001D Inspection Notes

- Explorer subagent performed read-only analysis.
- No build or test run was attempted because this was an analysis-only pass.
- Candidate ranking is based on apparent input/output clarity, platform coupling, and harness setup cost.

## Skipped Validation

Record skipped checks with the reason, date, and impact.

- 2026-05-30: Legacy Xcode build not attempted for the first harness step. The affected validation target is isolated C behavior and does not require the legacy app target.
