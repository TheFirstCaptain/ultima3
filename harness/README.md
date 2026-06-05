# Modernization Harness

This directory contains command-line characterization checks for bounded legacy behavior. It is intentionally separate from the legacy Xcode project so early modernization work does not require Carbon, QuickTime, legacy SDKs, resources, or a runnable app.

## Commands

```bash
make -C harness test
```

## Current Scope

- `map_math_accessors_tests.c` characterizes the map/math accessor behavior from `Sources/UltimaMisc.c`: `GetHeading`, `Absolute`, `GetXY`, `PutXY`, `MapConstrain`, `GetXYVal`, and `PutXYVal`.
- `pascal_string_helpers_tests.c` characterizes byte-level Pascal string helper behavior from `Sources/UltimaText.c`: `StringLocation`, `SearchReplace`, `IsNewline`, and `AddString`.
- `combat_predicates_tests.c` characterizes combat movement and lookup predicates from `Sources/UltimaSpellCombat.c`: `CombatValidMove`, `CombatMonsterHere`, and `CombatCharacterHere`.
- `combat_damage_tests.c` characterizes monster damage resolution from `Sources/UltimaSpellCombat.c`: `DamageMonster` HP subtraction, defeat threshold, Lord British immunity, experience event data, tile restoration, and redraw signaling.
- `combat_attack_tests.c` characterizes bounded player attack resolution from `Sources/UltimaSpellCombat.c`: post-direction `CombatAttack` targeting, melee and projectile weapon paths, thrown dagger inventory mutation, Exodus Castle restriction, hit chance, damage formula, hit/miss signaling, and handoff to `DamageMonster`.
- `combat_monster_action_tests.c` characterizes bounded monster action resolution from `Sources/UltimaSpellCombat.c`: movement application after `FigureNewMonPosition`, archer shooting, magic targeting, poison, pilfer, armour miss checks, Exodus Castle auto-hit behavior, player damage, death handling, tile restoration, and event signaling.
- `autocombat_targeting_tests.c` characterizes autocombat targeting, danger checks, and movement forecast helpers from `Sources/UltimaAutocombat.c`: `ThreatValue`, `MonsterCanAttack`, `NearlyDead`, `MonsterNearby`, `MonsterLinedUp`, `AutoMoveChar`, `SetupNow`, `SetupFuture`, `FutureMonsterHere`, `DirToNearestMonster`, and `LineUpToMonster`.
- `autocombat_targeting_tests.c` also characterizes the top-level `AutoCombat` macro decision flow as a portable bounded command sequence.
- `dungeon_navigation_tests.c` characterizes bounded dungeon navigation behavior from `Sources/UltimaDngn.c`: `Forward`, `Retreat`, `Left`, `Right`, `dDescend`, and `dKlimb`.
- `resource_fixture_tests.c` validates the read-only classic resource parser against `Resources/English.lproj/MainResources.rsrc`, confirms the default save-template records needed for fixture extraction, asserts archive inventory totals used by resource-planning docs, and characterizes representative map, talk, dungeon, and combat-screen fixture layouts.

These harnesses compile small shared portable implementations from `Core/src/` and keep references back to the legacy source in the core files. This keeps the validation path executable on a modern compiler while avoiding broad platform shims for unrelated legacy file dependencies.
