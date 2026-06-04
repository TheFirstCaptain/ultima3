# LairWare's Ultima III Modernization

This fork is a modernization project for LairWare's Ultima III, the legacy macOS/Xcode codebase originally published by Leon McNeill ("Beastie"). The source fork point is:

https://github.com/beastie/ultima3

The goal is to preserve the behavior and historical character of the original Mac game while moving the codebase toward a modern macOS application that can build and run on Apple silicon. The current work is focused on understanding the legacy system, characterizing behavior with tests, extracting portable game logic, and documenting each modernization decision before broad platform rewrites.

## Modernization Approach

The project treats the legacy source and bundled assets as the reference implementation. Rather than replacing large subsystems at once, the work proceeds in small, documented slices:

1. Inventory legacy responsibilities, platform dependencies, resources, and save formats.
2. Build harness coverage around bounded behavior that can run without Carbon, QuickDraw, QuickTime, or legacy resource loading.
3. Extract portable C game-core modules under `Core/` with explicit state and testable APIs.
4. Preserve platform-specific concerns behind future adapters for input, rendering, audio, resources, persistence, and the app shell.
5. Record feature status, defects, validation strategy, and durable decisions in markdown so future work can resume safely.

The command-line harness is the current validation path for extracted core behavior:

```bash
make -C harness test
```

The legacy Xcode target may not build on modern machines without old SDKs and frameworks. That is expected at this stage.

## Primary Documents

| Document | Purpose |
| --- | --- |
| [Decision Log](DECISION_LOG.md) | Durable architecture, compatibility, validation, and workflow decisions. |
| [Modernization Overview](docs/modernization/README.md) | Entry point for modernization planning documents. |
| [Legacy Inventory](docs/modernization/legacy-inventory.md) | Source responsibilities, platform dependencies, state coupling, asset formats, and save/resource inventory. |
| [Porting Strategy](docs/modernization/porting-strategy.md) | Sequencing notes and target boundaries for the modern port. |
| [Validation Strategy](docs/modernization/validation-strategy.md) | Harness strategy, validation levels, and covered characterization targets. |
| [Feature Tracker](docs/features/FEATURE_TRACKER.md) | Current feature status and links to feature documents. |
| [Bug Tracker](docs/bugs/BUG_TRACKER.md) | Known defects, regressions, blockers, and compatibility issues. |
| [Harness README](harness/README.md) | Notes for the command-line characterization harness. |

## Current Direction

Completed modernization work has established the harness, seeded portable core modules, and extracted/characterized map math, Pascal string helpers, combat predicates, autocombat behavior, dungeon navigation, and several combat action-resolution paths.

Recent planning work has focused on save and resource formats. The project has identified Resource Manager-backed roster/save data and `MainResources.rsrc` as major modernization blockers. Proposed next milestones include fixture extraction, resource archive enumeration, persistence adapter design, and map/talk/combat fixture characterization.

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
