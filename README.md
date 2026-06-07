# LairWare's Ultima III Modernization

This fork is a modernization project for LairWare's Ultima III, the legacy macOS/Xcode codebase originally published by Leon McNeill ("Beastie"). The source fork point is:

https://github.com/beastie/ultima3

The goal is to preserve the behavior and historical character of the original Mac game while moving the codebase toward a modern macOS application that can build and run on Apple silicon. The current work is focused on keeping legacy behavior characterized, extracting portable game logic, and growing a modern AppKit-owned shell through small adapter-backed milestones.

## Modernization Approach

The project treats the legacy source and bundled assets as the reference implementation. Rather than replacing large subsystems at once, the work proceeds in small, documented slices:

1. Inventory legacy responsibilities, platform dependencies, resources, and save formats.
2. Build harness coverage around bounded behavior that can run without Carbon, QuickDraw, QuickTime, or legacy resource loading.
3. Extract portable C game-core modules under `Core/` with explicit state and testable APIs.
4. Preserve platform-specific concerns behind future adapters for input, rendering, audio, resources, persistence, and the app shell.
5. Record feature status, defects, validation strategy, and durable decisions in markdown so future work can resume safely.

The command-line harness validates extracted core behavior:

```bash
make -C harness test
```

The modern shell is built with SwiftPM. The legacy Xcode target may not build on modern machines without old SDKs and frameworks. That is expected at this stage.

## Starting the Modern App

Build the modern shell:

```bash
swift build --product Ultima3ModernShell
```

Start it from SwiftPM:

```bash
swift run Ultima3ModernShell
```

The app launched by this command is the modernization shell, not the original legacy Xcode target.

## Primary Documents

| Document | Purpose |
| --- | --- |
| [Architecture](ARCHITECTURE.md) | Current and target architecture, including strict shell/core/adapter boundary rules. |
| [Implementation Boundary Checklist](BOUNDARIES.md) | Quick pre-implementation and pre-review checklist for shell, adapter, and core boundaries. |
| [Decision Log](DECISION_LOG.md) | Durable architecture, compatibility, validation, and workflow decisions. |
| [Modern App Shell Tech Comparison](MODERN_APP_SHELL_TECH_COMPARISON.md) | App shell technology comparison and accepted AppKit/SwiftUI direction. |
| [Modernization Overview](docs/modernization/README.md) | Entry point for modernization planning documents. |
| [Legacy Inventory](docs/modernization/legacy-inventory.md) | Source responsibilities, platform dependencies, state coupling, asset formats, and save/resource inventory. |
| [Porting Strategy](docs/modernization/porting-strategy.md) | Sequencing notes and target boundaries for the modern port. |
| [Validation Strategy](docs/modernization/validation-strategy.md) | Harness strategy, validation levels, and covered characterization targets. |
| [Feature Tracker](docs/features/FEATURE_TRACKER.md) | Current feature status and links to feature documents. |
| [Bug Tracker](docs/bugs/BUG_TRACKER.md) | Known defects, regressions, blockers, and compatibility issues. |
| [Harness README](harness/README.md) | Notes for the command-line characterization harness. |

## Current Direction

Completed modernization work has established the harness, seeded portable C core modules, and characterized map/math behavior, Pascal strings, combat predicates, combat action resolution, autocombat behavior, dungeon navigation, save/resource fixtures, resource inventory, persistence direction, representative map/talk/combat fixtures, shell resource/persistence integration, a byte-preserving new-game save-domain smoke state, automated coverage for a resource-backed render smoke frame, and automated coverage for a fixed shell tick smoke loop.

The first modern app shell now exists under `ModernShell/` as a SwiftPM executable product named `Ultima3ModernShell`. AppKit owns lifecycle, windows, menus, command routing, input intake, the game host view, fullscreen/window behavior, and future concrete save-file location. SwiftUI is currently limited to a preferences panel boundary.

The modern shell currently has early adapter smoke paths for rendering, input, audio, resources, persistence, new-game save-domain loading, and a fixed AppKit-owned tick:

- `GameHostView` renders a resource-backed portable frame from `CONS` 400 tile bytes through an AppKit renderer adapter, with a synthetic fallback if resources are unavailable.
- Keyboard, mouse, and menu commands pass through a portable input queue and are consumed by the shell tick.
- A shell-owned AVFoundation audio adapter consumes portable audio events on tick and can play bundled sound assets.
- Shell resource and save adapters validate bundled `MainResources.rsrc`, build and write native smoke save documents, and load the portable new-game save-domain state.

The next proposed backlog candidates F-024 through F-026 sketch the path toward party/roster state, overworld movement, and character creation.

## App Shell Boundary

- AppKit owns shell lifecycle, primary windows, menus, command routing, keyboard and mouse event intake, fullscreen/window behavior, the game host view, and concrete save-file location.
- SwiftUI is limited to preferences, inspectors, setup flows, and future non-game panels unless a later decision explicitly changes ownership.
- `Core/` public headers must not expose AppKit, SwiftUI, SDL, Metal, AVFoundation, Foundation filesystem, sandbox, or other platform framework types.
- Renderer, input, audio, resource, and persistence implementations remain behind adapters and communicate with the portable core through portable C data, result structs, events, command buffers, or explicit state transitions.
- The shell may provide concrete bundle and save-file URLs or paths, but resource parsing and persistence serialization remain adapter-owned.

## Original Upstream README

# LairWare's Ultima III

This started out as an unofficial fan remake of the original 1983 Apple II game from Origin Systems.  Origin had made official Mac ports of a few older Ultima games, but these were all monochrome.  My remake was originally implemented in Think C for 1990s-era color Macintosh computers on Motorola processors running Mac OS 7.  I really liked how it was turning out, so I managed to get ahold of Richard Garriott over AOL and he liked it enough to give me permission to release it officially sometime in 1994 or 1995.

Some of the logic was originally gleaned through examining the Apple II version's 6502 assembly code.  You can find comments throughout the source referring to memory locations in this version!  There were no such things as "shrinkwrap" licenses back then which would forbid such reverse engineering.

In my spare time over the following 10+ years I would poke and prod at it to keep it running on current systems of the time; making it capable of running on Mac OS X without the need for Classic, compiling it for Intel processors to eliminate the need for Rosetta, adding support for alternate graphics, etc.  I had transitioned the project to CodeWarrior early on, then to Xcode when that came out.  By the time macOS Catalina was released with its removal of support for 32-bit executables, I had only barely touched this project for many many years.

For upload, I've mostly removed license key handling and update checking.  I haven't checked if it still compiles!  I keep telling myself that this isn't intended to be useful to anyone, it's just some code archaeology.

_Random fun fact: Ultima III was one of the first games to acknowledge non-binary gender!_

## License
Usage is provided under the [MIT License](http://opensource.org/licenses/mit-license.php). See LICENSE for the full details.

However, certain non-code assets (such as the project name, music, maps, etc) were not originally created by me. These assets are included under the assumption that copyright will no longer be actively enforced due to their age (40+ years). If you are a rights-holder and have concerns, please contact me.

To put it another way: I'm not claiming any copyright on the Ultima franchise name, NPC names, the specific maps found in this game, etc.  This license just refers to everything else here.  I'm presenting it merely as historical code in good faith, in hope that no one will care to litigate -- there is indeed no feasible way I am aware of to build this project to run on a modern system without an emulator.

Leon McNeill AKA "Beastie"
