# Architecture

## Modernization Goal

The target architecture is a modern macOS application that builds and runs on Apple silicon Macs. The port should preserve the original Ultima III behavior where practical while replacing unsupported legacy dependencies such as Carbon, QuickTime, classic Mac resource assumptions, and old SDK/toolchain requirements.

## Current Architecture

This repository currently contains a monolithic legacy Mac application written mostly in C with Objective-C bridge and UI code. The code uses shared global state, classic Mac event handling, Carbon windows and drawing, QuickTime audio, resource-based assets, NIB UI, and an old Xcode project configuration.

## Repository Topology

- `Sources/`: legacy application source, including game logic, rendering, input, audio, platform glue, and Objective-C bridge code.
- `Resources/`: bundled game strings, sounds, music, graphics, icon, and legacy resource files.
- `English.lproj/`: localized app metadata and `GameOptions.nib`.
- `Images/`: reference PDFs for commands, spells, and tables.
- `Cursors/`: cursor notes and related material.
- `Ultima3.xcodeproj/`: legacy Xcode project metadata.

## Current Runtime Shape

```text
main.m
  -> legacy Mac application startup
  -> UltimaMain.c main loop
  -> shared global game state
  -> game systems
       -> map, dungeon, combat, spells, text
  -> platform services
       -> Carbon events, windows, and drawing
       -> QuickTime sound and music
       -> Mac resources, NIBs, and files
```

## Legacy System Areas

- Startup and main loop: `main.m`, `UltimaMain.c`
- Mac interface and platform glue: `UltimaMacIF.c`, `CarbonShunts.c`, `CocoaBridge.m`
- Graphics and drawing: `UltimaGraphics.c`, `UltimaGraphics.h`
- Sound and music: `UltimaSound.m`, `LWSoundPlayer.m`
- Game systems: `UltimaNew.c`, `UltimaDngn.c`, `UltimaAutocombat.c`, `UltimaSpellCombat.c`
- Text and resources: `UltimaText.c`, `Resources/English.lproj/Strings/`
- Preferences and dialogs: `PrefsDialog.m`, `PrefsDialog.h`, `English.lproj/GameOptions.nib`
- Apple events: `UltimaAppleEvents.c`

## Target Architecture

The modernization target is a modern macOS app shell around an isolated, testable game core. Platform-specific behavior should move behind adapters so the game rules and state transitions can be characterized and ported independently from rendering, input, audio, and persistence.

```text
Modern macOS app shell
  -> input adapter
  -> renderer adapter
  -> audio adapter
  -> resource/persistence adapter
  -> portable game core
       -> world and map state
       -> dungeon state
       -> combat
       -> spells
       -> text and messages
       -> save/load state transitions
```

## Target Boundary Rules

- The portable game core should not call macOS UI, audio, filesystem, resource, or rendering APIs directly.
- Platform adapters should translate modern app events and services into core-facing interfaces.
- Legacy code remains the reference implementation until behavior is characterized or intentionally changed.
- State mutations should become explicit and testable over time.
- Asset conversion should be documented and reproducible when historical formats are replaced.

## Portable Core Layout

The initial shared portable game core will live under a new top-level `Core/` directory rather than inside legacy `Sources/`:

```text
Core/
  include/
    u3_<domain>.h
  src/
    u3_<domain>.c
```

The core is plain C. Public headers and functions use a lowercase `u3_` prefix, constants use `U3_<DOMAIN>_<NAME>`, and stateful legacy globals should become small caller-owned structs passed explicitly into functions. Core headers should use fixed-width standard C types and should not expose Classic Mac types such as `Str255`; exact Pascal string behavior can use a portable 256-byte alias owned by the relevant core header. Harness-only shims remain in `harness/`.

## Migration Strategy

1. Inventory legacy files, globals, data formats, and platform API touchpoints.
2. Characterize behavior with focused harnesses or tests where practical.
3. Extract or port one bounded subsystem at a time.
4. Replace platform dependencies with adapters.
5. Verify behavior through automated tests where available and manual gameplay checks where needed.

## Known Coupling & Risks

- Heavy global state, especially around `UltimaMain.c`.
- Classic Mac types appear throughout the codebase, including `Rect`, `WindowPtr`, `CGrafPtr`, `Handle`, and `Str255`.
- Rendering, input handling, resource loading, and game state may be tightly coupled.
- QuickTime and Carbon dependencies are unsupported on modern macOS.
- Some behavior may only be discoverable by reading code because the legacy app may not run.
- Historical asset licensing and conversion choices need care.

## Open Questions

- Should the modern shell use AppKit, SwiftUI, SDL, or another approach?
- Should the portable game core remain in C, move to C++, or be ported to Swift?
- Which renderer should own tiles, animation, scaling, and color management?
- Should legacy save files remain compatible?
- Which assets need conversion, and what source-of-truth format should replace legacy resources?
- What is the first subsystem suitable for characterization?
