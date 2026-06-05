# Modern App Shell Tech Comparison

Purpose: provide an offline decision aid for F-011 before recording the modern app shell direction. This document compares candidate shell technologies against the needs of this specific Ultima III modernization effort. It does not choose the final direction.

## Application Needs

| Need | Why it matters for this app |
| --- | --- |
| Apple silicon native build | The accepted modernization target is a modern macOS app that runs natively on Apple silicon while leaving the legacy Xcode project intact. |
| C portable core integration | Current extraction work is plain C under `Core/`, and the shell should host that core without forcing a language rewrite. |
| Deterministic game loop / frame ticks | The game needs predictable input, movement, animation, combat, and redraw timing. |
| Keyboard and mouse command fidelity | The original Mac app uses dense keyboard commands, menus, mouse/cursor affordances, and game command routing. |
| Classic menu and command model | The port should preserve native Mac expectations around menus, commands, dialogs, preferences, and app lifecycle. |
| Preferences UI | Existing preferences/dialog concepts need a modern replacement without coupling preferences to the portable core. |
| 2D tile rendering path | The game is tile-based and needs a clear route for drawing maps, combat screens, text, cursors, and UI panels. |
| Future Metal or GPU path | A first shell should not block a later GPU-backed renderer if scaling, animation, or performance needs grow. |
| Audio adapter replacement | QuickTime/Sound Manager paths need replacement behind an adapter. |
| Bundle resource access | Converted or parsed resources need straightforward bundled-file access. |
| Save-file provider for F-014 boundary | F-014 puts concrete save-file location ownership in the app shell while keeping persistence serialization in an adapter. |
| Native windowing/fullscreen behavior | The modern app should feel like a normal macOS app, including windows, focus, fullscreen, and restoration behavior. |
| macOS accessibility and app conventions | Native app behavior, accessibility surfaces, keyboard focus, and expected system integration reduce long-term UI risk. |
| Cross-platform potential | Portability is useful only if it does not undermine the current macOS-first target. |
| Migration from current Objective-C bridge code | The legacy app already has Objective-C/Cocoa bridge code, Mac menus, preferences, and dialogs. |
| Long-term maintainability for this repo | The shell should fit the existing C/Objective-C history and the adapter-first modernization plan. |

## Tier Scale

| Tier | Meaning |
| --- | --- |
| T4 | Strong fit: meets the need directly with low expected adapter risk. |
| T3 | Good fit: meets the need with manageable integration or implementation work. |
| T2 | Usable with caveats: feasible, but likely needs wrappers, custom work, or careful constraints. |
| T1 | Weak fit: possible, but risks awkward integration, behavior drift, or high maintenance. |
| T0 | Poor fit: does not meet the need without major extra architecture or a separate subsystem. |

## Candidate Summary

| Candidate | Summary |
| --- | --- |
| AppKit-first native shell | AppKit owns the app lifecycle, windowing, menus, commands, preferences surfaces, event handling, and host views. Renderer, audio, resource, and persistence stay behind adapters. |
| SwiftUI-first native shell | SwiftUI owns app lifecycle and views first, with AppKit or lower-level host views wrapped only where needed. |
| AppKit plus SwiftUI hybrid | AppKit owns shell, menus, windowing, command routing, game host, and lifecycle. SwiftUI is adopted selectively for preferences and secondary panels. |
| SDL3 shell | SDL owns the window, input, renderer/audio abstraction, and event loop. Native macOS behavior is added only where required. |
| SDL3 inside AppKit hybrid | AppKit owns native app behavior while an SDL-backed surface owns low-level game rendering and input. |

## Needs Matrix

| Need | AppKit-first | SwiftUI-first | AppKit + SwiftUI | SDL3 shell | SDL3 in AppKit |
| --- | --- | --- | --- | --- | --- |
| Apple silicon native build | T4 | T4 | T4 | T3 | T3 |
| C portable core integration | T4 | T3 | T4 | T4 | T4 |
| Deterministic game loop / frame ticks | T3 | T2 | T3 | T4 | T3 |
| Keyboard and mouse command fidelity | T4 | T2 | T4 | T4 | T4 |
| Classic menu and command model | T4 | T2 | T4 | T1 | T4 |
| Preferences UI | T3 | T4 | T4 | T1 | T3 |
| 2D tile rendering path | T3 | T2 | T3 | T4 | T4 |
| Future Metal or GPU path | T3 | T2 | T3 | T4 | T4 |
| Audio adapter replacement | T3 | T3 | T3 | T4 | T4 |
| Bundle resource access | T4 | T4 | T4 | T2 | T3 |
| Save-file provider for F-014 boundary | T4 | T4 | T4 | T2 | T4 |
| Native windowing/fullscreen behavior | T4 | T3 | T4 | T2 | T4 |
| macOS accessibility and app conventions | T4 | T4 | T4 | T1 | T3 |
| Cross-platform potential | T1 | T2 | T1 | T4 | T3 |
| Migration from current Objective-C bridge code | T4 | T2 | T4 | T1 | T3 |
| Long-term maintainability for this repo | T3 | T3 | T4 | T3 | T3 |

## AppKit-First Native Shell

Best fit when native macOS behavior, command routing, menus, preferences, file handling, and direct C/Objective-C integration matter more than cross-platform portability.

Strengths:

- Natural match for modern macOS windowing, menu commands, dialogs, focus, app lifecycle, and save-file location ownership.
- Fits the repository history: `Sources/CocoaBridge.m`, `Sources/PrefsDialog.m`, `Sources/UltimaMacIF.c`, and related Objective-C/Cocoa bridge code already point in this direction.
- Straightforward bridge to a plain C portable core.
- Keeps resource and persistence adapters independent of shell-specific storage APIs.

Weaknesses:

- AppKit does not solve rendering or audio by itself.
- A custom game view still needs a deliberate rendering path, likely Core Graphics, bitmap-backed drawing, Metal, or a staged adapter that can evolve.
- Cross-platform value is low unless the shell is later replaced or separated from a shared renderer/core.

Good F-011 answer if:

- Native Mac behavior is a primary goal.
- The first shell should minimize lifecycle and menu risk.
- A renderer adapter can be chosen separately after the shell decision.

## SwiftUI-First Native Shell

Best fit when declarative UI and rapid modern preference/panel development matter more than custom game-surface ownership.

Strengths:

- Strong fit for preferences, inspectors, setup panels, and other non-game UI.
- Modern Apple UI direction and easy incremental UI composition.
- Can interoperate with AppKit when a lower-level host view is needed.

Weaknesses:

- Weaker first owner for dense keyboard command handling, custom rendering, deterministic frame ticks, and legacy-style menu command routing.
- A game view would likely need AppKit, Metal, or another lower-level wrapper early.
- Risk of pushing core game concerns through wrappers before adapter contracts are stable.

Good F-011 answer if:

- The port prioritizes modern declarative UI over preserving a classic Mac command model in the first shell.
- The team accepts early wrapper work for the game surface.

## AppKit Plus SwiftUI Hybrid

Best fit when the app should be native Mac first while still allowing SwiftUI for modern secondary UI.

Strengths:

- AppKit owns the hard shell parts: lifecycle, windows, menu commands, keyboard routing, game host view, save-file provider, and native conventions.
- SwiftUI can be introduced incrementally for preferences, inspectors, and future non-game panels.
- Avoids forcing SwiftUI to own the deterministic game surface before rendering/input adapters are clear.
- Strong match for the adapter-first modernization plan and current Objective-C/C bridge history.

Weaknesses:

- Needs a clear ownership rule so SwiftUI remains a panel/UI tool rather than a second competing shell.
- Still requires separate renderer and audio adapter decisions.
- Lower cross-platform value than SDL-based shells.

Good F-011 answer if:

- Native Mac conventions matter most.
- The first shell should preserve command/menu behavior while leaving room for modern SwiftUI preferences.
- The project wants a conservative bridge from legacy Objective-C/C code to a modern shell.

## SDL3 Shell

Best fit when cross-platform game portability and unified low-level input/render/audio abstractions matter more than native Mac app conventions.

Strengths:

- Strong game-loop, input, renderer, and audio abstraction story.
- Good path for portability beyond macOS.
- Natural fit for custom 2D game rendering and future GPU-backed paths.

Weaknesses:

- Weakest fit for native macOS menus, preferences, save-file presentation, accessibility, document handling, and app conventions.
- More native Mac behavior would need to be rebuilt, bridged, or accepted as reduced scope.
- Poor match for the current macOS-first modernization target unless portability becomes a top requirement.

Good F-011 answer if:

- Cross-platform portability becomes a primary goal.
- The project is willing to trade native Mac app feel for a stronger game-platform abstraction.

## SDL3 Inside AppKit Hybrid

Best fit when native Mac shell behavior is required but the game surface should use SDL's input/rendering/audio ecosystem.

Strengths:

- Keeps AppKit in charge of menus, windowing, lifecycle, save-file location, preferences, and app conventions.
- Gives the game surface a portable SDL-oriented rendering and input path.
- Stronger portability fallback than pure AppKit or AppKit plus SwiftUI.

Weaknesses:

- Two lifecycle and event systems can become awkward.
- Integration complexity may be higher than either a pure native shell or pure SDL shell.
- Needs careful boundaries so SDL does not fight AppKit for focus, input, window, fullscreen, or run-loop ownership.

Good F-011 answer if:

- Native Mac shell behavior is required, but renderer/input portability is important enough to justify hybrid lifecycle complexity.
- A prototype specifically proves the AppKit/SDL embedding model is stable.

## Decision Prompts

Use these prompts when reading the matrix offline:

1. Is the first modern target definitely macOS-native, or should portability materially influence the first shell?
2. Should the first shell preserve the original Mac menu/preferences model as a core requirement?
3. Is SwiftUI valuable enough in the first milestone to include immediately, or can it wait until preferences/secondary panels are built?
4. Should rendering start as a native AppKit-hosted view, an SDL-backed surface, or stay undecided until a renderer adapter spike?
5. Does the app need a prototype before accepting a hybrid approach with two lifecycle systems?
6. Is the save-file provider responsibility from F-014 easier to satisfy in a native shell than in an SDL shell?

## Practical Reading

If native macOS behavior is weighted highest, the leading candidates are `AppKit plus SwiftUI hybrid` and `AppKit-first native shell`.

If cross-platform rendering/input portability is weighted highest, the leading candidates are `SDL3 shell` and `SDL3 inside AppKit hybrid`.

If the project wants the lowest-risk first shell for the current repository shape, `AppKit plus SwiftUI hybrid` appears strongest because it keeps AppKit in charge of shell behavior while leaving SwiftUI available for secondary UI. This is a preliminary read only; F-011 still owns the final accepted or deferred decision.

## Source Trail

- `docs/features/F-011.md`: parent decision spike and acceptance notes.
- `docs/features/F-011A.md`: tiered shell options matrix.
- `ARCHITECTURE.md`: target boundary of modern macOS shell plus portable game core.
- `DECISION_LOG.md`: accepted Apple silicon macOS target and adapter-first modernization rules.
- `docs/features/F-014.md`: persistence adapter direction; app shell provides concrete save-file location.
- `docs/features/F-012.md`, `docs/features/F-013.md`, `docs/features/F-015.md`: resource and fixture characterization supporting the app-shell/resource boundary.

## Reference Links

- Apple AppKit documentation: https://developer.apple.com/documentation/appkit
- Apple SwiftUI overview: https://developer.apple.com/swiftui/
- Apple Metal documentation: https://developer.apple.com/documentation/metal
- Apple universal macOS binary documentation: https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary
- Apple AVFoundation overview: https://developer.apple.com/av-foundation/
- SDL3 wiki: https://wiki.libsdl.org/
- SDL3 platforms page: https://wiki.libsdl.org/SDL3/README-platforms
- SDL3 Metal renderer API: https://wiki.libsdl.org/SDL3/SDL_GetRenderMetalCommandEncoder
