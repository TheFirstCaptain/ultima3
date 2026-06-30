# Modern Shell

`ModernShell` is the first AppKit-owned macOS shell scaffold for the Ultima III modernization effort.

AppKit owns lifecycle, menus, command routing, the primary window, and the game host view. SwiftUI is currently limited to a preferences panel boundary. The shell links the plain C `Ultima3Core` Swift package target to prove that modern shell code can call portable core code without exposing AppKit or SwiftUI types through `Core/`.

`GameHostView` renders a `u3_render_frame` through `AppKitRenderAdapter`. The default smoke frame is built from the native new-game current Sosaria map fixture derived from `Resources/English.lproj/MainResources.rsrc`; the shell falls back to the synthetic tile frame if bundled resources are unavailable. The portable core describes clear, rectangle, tile, and dungeon perspective commands with plain C data, while AppKit owns the concrete drawing.

Keyboard, mouse, and menu actions pass through `ShellInputAdapter`, which translates AppKit-owned events into the portable `u3_input_queue`. Arrow keys, keypad `8/2/4/6`, and top-row `8/2/4/6` enqueue movement commands. The shell tick consumes queued input events, applies wrapped save-backed Sosaria movement and non-ship terrain passability through `u3_overworld`, then translates portable feedback fields into status text, redraws, and audio. Successful Sosaria movement writes updated party x/y bytes into the current native save document, redraws the centered viewport, reports `Move`, and requests `Step`; blocked terrain reports `Blocked`, includes the target tile value, and requests `Bump`. Handled movement attempts increment the legacy `PRTY` move counter by active party size, including blocked attempts.

Pressing `E` on a recognized town asks portable `u3_location` rules to load copied, immutable destination `MAPS`, `MONS`, and `TLKS` data through `ShellResourceAdapter`. Successful entry transactionally changes the in-memory `PRTY` location mode to town and applies the shared move-counter cost while retaining the Sosaria map, creature data, and exact return coordinate. Town and castle sessions use separate non-wrapping location movement: the transient session position and centered frame update, legacy-passable fixture tiles move, and blocking tiles request `Bump`. Reaching a legacy zero-coordinate exit transactionally restores Sosaria mode, the exact prior coordinate, the retained Sosaria frame, and overworld input routing. Location movement never writes destination coordinates over the saved Sosaria x/y bytes. Game > Refresh Locations retains and redraws an active transient session; New Game or Load Game replaces it with the selected Sosaria document. Macro command sequences use the same queue in legacy execution order.

Town Talk uses a reduced first-slice adaptation of legacy Transact: press `T`, then a cardinal direction. The legacy party-member prompt is deferred because this read-only subset does not use the selected character. Portable `u3_location_talk` rules accept only validated 64×64 town sessions, search copied `MONS` slots in legacy reverse priority, select an adjacent NPC, resolve the low-nibble talk index, and decode one null-delimited `TLKS` entry into a length-bounded result. The supported subset includes printable bytes, high-bit printable bytes after legacy masking, and `0xFF` line breaks. The shell displays the bounded text in status, including the characterized LCB Towne guard line `HO! HO!`; empty targets, invalid indices, unsupported control entries, and malformed records report safe explicit results. This slice is read-only and does not consume a turn. Generic greetings, character selection, speech, shops, branching dialogue, and talk-triggered mutations remain deferred.

Dungeon entry currently supports one characterized fixture: Dungeon Doom at Sosaria `(19,57)`, location index `12`, resource id `412`. Explicit Enter validates and copies exactly 2048 `MAPS` bytes and the 256-byte `TLKS` record, then transactionally sets in-memory dungeon party mode and initializes a transient session at level zero, `(1,1)`, heading east. The retained Sosaria map, creature data, and return coordinate remain untouched. Save is rejected while the dungeon session is active, and failed entry retains the outdoor document and frame. The active dungeon frame is derived by `u3_dungeon_make_view_frame` from explicit dungeon bytes, level, position, and heading, then rendered by the AppKit adapter as deterministic first-person geometry. Walls, doors, current-cell ladders, and current-cell chests are visually distinct; pixel-perfect legacy dungeon sprites, torch effects, dungeon input, movement, talk, and exit remain deferred.

`ShellAudioAdapter` is the first modern audio backend spike. It consumes portable `u3_audio_queue` events on tick and uses AVFoundation inside the shell to play existing sound assets for smoke testing. Missing or unsupported assets are reported in shell status text instead of changing the portable core contract.

`ShellLocationProvider` owns the first concrete resource-root and save-document path candidates for the AppKit shell. `ShellResourceAdapter` uses the shell-provided resource root to read `Resources/English.lproj/MainResources.rsrc`, validate known fixture records through the portable `u3_resource` parser, build the resource-backed render smoke frame, build/load the native new-game document, load the current Sosaria map and party position, load adapter-owned transient location records, and inspect the portable party/roster summary. The portable location descriptor contains validated metadata and lengths but no Foundation or resource-file pointers. `ShellSaveAdapter` stages, validates, atomically replaces, byte-verifies, and returns validated native save documents from the shell-owned save path.

The first gameplay save policy is manual save only. Game > New Game creates an in-memory current document, Game > Save atomically writes it only while the validated party state is in Sosaria, and Game > Load Game replaces the current document with the validated durable document. The shell restores a valid saved game at launch when one exists. Movement, town entry, character creation, and party assembly mark the current document as having unsaved changes but do not autosave. Save is clearly rejected inside a transient town; returning to Sosaria makes the accumulated turn changes safe to save. Starting or loading another game requires confirmation before discarding unsaved changes. Quitting with unsaved changes offers Save and Quit, Quit Without Saving, or Cancel; Save and Quit from town is rejected and cancels termination, while Quit Without Saving follows the existing discard policy. Autosave remains deferred. Game > Inspect Party/Roster prefers the current document when loaded. The game host status overlay reports command, tick, resource/render, save state, and whether changes are unsaved; Game > Refresh Locations redraws the active location view without discarding its transient session, Game > Create Character... opens the character setup panel, Game > Assemble Party... opens the party setup panel, and Game > Toggle Tick pauses or resumes the fixed timer.

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
