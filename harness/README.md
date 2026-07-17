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
- `combat_player_command_tests.c` characterizes the first bounded player combat command contract from `Sources/UltimaSpellCombat.c`: cardinal movement, blocked movement, pass, attack direction prompting, melee hit/miss handoff, redraw/audio signaling, and explicit projectile/deferred status.
- `combat_monster_action_tests.c` characterizes bounded monster action resolution from `Sources/UltimaSpellCombat.c`: movement application after `FigureNewMonPosition`, archer shooting, magic targeting, poison, pilfer, armour miss checks, Exodus Castle auto-hit behavior, player damage, death handling, tile restoration, and event signaling.
- `combat_render_tests.c` validates the portable combat arena render contract from explicit `CONS` bytes and occupant state, including terrain command layout, deterministic party/monster overlays, tile-only fallback, invalid screen reporting, invalid occupant skipping, and null-state fallback.
- `autocombat_targeting_tests.c` characterizes autocombat targeting, danger checks, and movement forecast helpers from `Sources/UltimaAutocombat.c`: `ThreatValue`, `MonsterCanAttack`, `NearlyDead`, `MonsterNearby`, `MonsterLinedUp`, `AutoMoveChar`, `SetupNow`, `SetupFuture`, `FutureMonsterHere`, `DirToNearestMonster`, and `LineUpToMonster`.
- `autocombat_targeting_tests.c` also characterizes the top-level `AutoCombat` macro decision flow as a portable bounded command sequence.
- `dungeon_navigation_tests.c` characterizes bounded dungeon navigation behavior from `Sources/UltimaDngn.c`: `Forward`, `Retreat`, `Left`, `Right`, `dPass`, `dDescend`, and `dKlimb`.
- `dungeon_render_tests.c` validates the portable dungeon perspective frame contract from explicit dungeon bytes, including heading-relative sampling, near-to-far walls, doors, current-cell ladder/chest feature commands, invalid input fallback, light-limited rendering, light decay, and no source-buffer mutation.
- `dungeon_special_tile_tests.c` validates dungeon tile-effect contracts for strange wind, traps, gremlin food theft, writing/deferred tile reporting, invalid inputs, deterministic party-member selection, bounded HP/status mutation, food underflow prevention, prompt-backed fountain variants, mark bit/HP costs, Time Lord/message status, chest opening/reward, cancellation, and deferred chest traps.
- `dungeon_post_turn_tests.c` characterizes passive dungeon post-command mechanics from `Sources/UltimaDngn.c`: aging/status refresh requests, light decay, empty-floor encounter probability, level-bounded monster type selection, and deferred combat-screen metadata.
- `resource_fixture_tests.c` validates the read-only classic resource parser against `Resources/English.lproj/MainResources.rsrc`, confirms the default save-template records needed for fixture extraction, asserts archive inventory totals used by resource-planning docs, and characterizes representative map, talk, dungeon, and combat-screen fixture layouts.
- `save_fixture_tests.c` validates the native new-game save fixture builder against default templates from `Resources/English.lproj/MainResources.rsrc`, including explicit party, roster, current Sosaria map, zero-filled current Sosaria creature, mutable `MISC`, and metadata records. It also smoke-tests temp-path atomic write/read behavior and invalid replacement retention outside `Core/`, and imports a synthesized mutable legacy roster resource fixture into the native save-domain document.
- `location_transition_tests.c` validates town and Dungeon Doom requests/session fixtures plus explicit party-mode entry and exact Sosaria return mutations, including shared entry turn cost, first-slice dungeon restriction, exact dungeon initialization, and rejection without partial mutation.
- `location_talk_tests.c` validates directional adjacent-NPC selection, reverse slot priority, bounded `TLKS` decoding, a stable LCB Towne guard line, no-mutation behavior, and safe empty, malformed, invalid-index, and unsupported-control results.
- `render_contract_tests.c` characterizes the portable renderer frame contract, including logical dimensions, 11x11 tile-grid command layout, command capacity, and synthetic frame generation.
- `input_adapter_tests.c` characterizes the portable input event queue shared by keyboard, mouse, menu, and macro-driven command events.
- `audio_adapter_tests.c` characterizes the portable audio event queue shared by sound, music, stop, and volume events, including the TorchIgnite sound id used by dungeon light.
- `tick_smoke_tests.c` characterizes the portable smoke tick state used by the modern shell heartbeat.
- `party_roster_tests.c` validates portable party/roster summary decoding from the native save-domain state plus bounded torch inventory mutation for dungeon Ignite, including legacy good/poisoned living status handling.
- `overworld_movement_tests.c` validates the first bounded overworld smoke movement rule and the resource-map render frame party marker.
- `character_creation_tests.c` validates the first portable character candidate rules used by the modern setup-flow spike.

These harnesses compile small shared portable implementations from `Core/src/` and keep references back to the legacy source in the core files. This keeps the validation path executable on a modern compiler while avoiding broad platform shims for unrelated legacy file dependencies.
