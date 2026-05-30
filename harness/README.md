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

These harnesses compile small shared portable implementations from `Core/src/` and keep references back to the legacy source in the core files. This keeps the validation path executable on a modern compiler while avoiding broad platform shims for unrelated legacy file dependencies.
