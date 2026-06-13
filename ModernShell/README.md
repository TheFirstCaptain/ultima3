# Modern Shell

`ModernShell` is the first AppKit-owned macOS shell scaffold for the Ultima III modernization effort.

AppKit owns lifecycle, menus, command routing, the primary window, and the game host view. SwiftUI is currently limited to a preferences panel boundary. The shell links the plain C `Ultima3Core` Swift package target to prove that modern shell code can call portable core code without exposing AppKit or SwiftUI types through `Core/`.

`GameHostView` renders a `u3_render_frame` through `AppKitRenderAdapter`. The default smoke frame is built from the native new-game current Sosaria map fixture derived from `Resources/English.lproj/MainResources.rsrc`; the shell falls back to the synthetic tile frame if bundled resources are unavailable. The portable core describes clear, rectangle, and tile commands with plain C data, while AppKit owns the concrete drawing.

Keyboard, mouse, and menu actions pass through `ShellInputAdapter`, which translates AppKit-owned events into the portable `u3_input_queue`. Arrow keys, keypad `8/2/4/6`, and top-row `8/2/4/6` enqueue overworld movement commands. The shell tick consumes queued input events, applies wrapped save-backed movement and non-ship terrain passability through `u3_overworld`, then translates portable feedback fields into status text, redraws, and audio. Successful movement writes the updated party x/y bytes into the current native save document, redraws the centered Sosaria viewport, reports `Move`, and requests `Step`; blocked terrain reports `Blocked`, includes the target tile value, and requests `Bump`. Macro command sequences use the same queue in legacy execution order.

`ShellAudioAdapter` is the first modern audio backend spike. It consumes portable `u3_audio_queue` events on tick and uses AVFoundation inside the shell to play existing sound assets for smoke testing. Missing or unsupported assets are reported in shell status text instead of changing the portable core contract.

`ShellLocationProvider` owns the first concrete resource-root and save-document path candidates for the AppKit shell. `ShellResourceAdapter` uses the shell-provided resource root to read `Resources/English.lproj/MainResources.rsrc`, validate known fixture records through the portable `u3_resource` parser, build the resource-backed render smoke frame, build/load the native new-game smoke document, load the current Sosaria map and party position, and inspect the portable party/roster summary. `ShellSaveAdapter` stages, validates, atomically replaces, reads back, and returns validated native save documents from the shell-owned save path. The shell now owns an optional current native save document: Game > Load New Game Smoke seeds it from bundled resources, Game > Load Saved Smoke reads it from disk, Game > Write Save Smoke atomically writes the current document, and Game > Inspect Party/Roster prefers the current document when loaded. The game host status overlay reports command, tick, resource/render, and save-path smoke state; Game > Refresh Locations reruns the location/resource/render smoke, Game > Create Character... opens the character setup panel, Game > Assemble Party... opens the party setup panel, and Game > Toggle Tick pauses or resumes the fixed timer.

`CharacterCreationView` is a SwiftUI setup panel hosted and owned by the AppKit shell. It validates a character candidate through the portable `u3_character` rules and, when a current native save document is loaded, writes accepted candidates into the first empty `ROST` slot with legacy first-character defaults. Active party selection remains separate from character creation.

`PartyAssemblyView` is a SwiftUI setup panel hosted and owned by the AppKit shell. It lists occupied `ROST` slots from the current native save document, tracks the selected order, and writes one to four selected roster ids into the native `PRTY` record through `u3_party_form_from_roster`. Replacing an existing active party requires explicit confirmation.

Build the shell with:

```bash
swift build --product Ultima3ModernShell
```

Run shell adapter tests with:

```bash
swift test
```

Run the shell executable with:

```bash
swift run Ultima3ModernShell
```

This scaffold intentionally does not choose the final renderer, final audio mixing policy, resource conversion path, persistence format, or gameplay loop.
