# Legacy Inventory

Use this document to inventory legacy files, data formats, platform dependencies, and behavior before porting.

## Source Areas

| Area | Files | Notes |
| --- | --- | --- |
| Startup and main loop | `Sources/main.m`, `Sources/UltimaMain.c`, `Sources/UltimaMain.h` | `main.m` delegates to `Ultima3_main()`. `UltimaMain.c` owns startup sequencing, main/menu/game loops, command dispatch, player actions, stats, movement, key/mouse waiting, and many central globals. |
| Game state | `Sources/UltimaMain.c`, `Sources/UltimaMain.h`, `Sources/UltimaIncludes.h` | Central state is mostly global arrays and scalars declared in `UltimaMain.c`, including `Player`, `Party`, `Monsters`, `Dungeon`, `TileArray`, map/time/moon fields, character/monster positions, and UI/platform state. `UltimaIncludes.h` defines shared constants and preference keys. |
| Map and dungeon | `Sources/UltimaMisc.c`, `Sources/UltimaMisc.h`, `Sources/UltimaDngn.c`, `Sources/UltimaDngn.h`, `Sources/UltimaNewMap.c`, `Sources/UltimaNewMap.h` | `UltimaMisc.c` owns map accessors, map loading, monster movement/spawning, Sosaria state, shops, shrines, roster/party persistence helpers, and miscellaneous commands. `UltimaDngn.c` owns dungeon navigation, dungeon drawing, dungeon info, and dungeon graphics. `UltimaNewMap.c` appears to generate and draw a new/diorama map. |
| Combat and spells | `Sources/UltimaSpellCombat.c`, `Sources/UltimaSpellCombat.h`, `Sources/UltimaAutocombat.c`, `Sources/UltimaAutocombat.h` | `UltimaSpellCombat.c` owns spell selection/casting, combat loop, projectiles, monster damage/movement, valid moves, dungeon cell helpers, victory, poison, and hit effects. `UltimaAutocombat.c` owns automated combat decisions and future monster-position prediction. |
| Text and messages | `Sources/UltimaText.c`, `Sources/UltimaText.h`, `Resources/English.lproj/Strings/` | `UltimaText.c` owns text rendering, message printing, input fields, string wrapping/search/replace, item/tile/monster names, and speech handoff. Text content appears to come from localized plist/string resources. |
| Graphics and rendering | `Sources/UltimaGraphics.c`, `Sources/UltimaGraphics.h`, `Sources/UltimaNewMap.c`, `Sources/UltimaDngn.c` | `UltimaGraphics.c` owns frame/tile drawing, named image loading, map/minimap rendering, intro/demo/exodus imagery, scroll areas, masks, GWorld copy setup, portraits, and display updates. Dungeon and map modules also draw directly. |
| Input and events | `Sources/UltimaMain.c`, `Sources/UltimaMacIF.c`, `Sources/UltimaMacIF.h`, `Sources/UltimaAppleEvents.c`, `Sources/UltimaAppleEvents.h`, `Sources/CarbonShunts.c`, `Sources/CarbonShunts.h` | `UltimaMain.c` waits for keyboard/mouse input and dispatches in-game commands. `UltimaMacIF.c` owns menus, mouse handling, alerts/dialog filters, macro input, cursor updates, window/menu behavior, pause/hibernate/wake, and app/system setup. `UltimaAppleEvents.c` registers Apple event handlers. |
| Sound and music | `Sources/UltimaSound.m`, `Sources/UltimaSound.h`, `Sources/LWSoundPlayer.m`, `Sources/LWSoundPlayer.h`, `Sources/CocoaBridge.m`, `Resources/Sounds/`, `Resources/Music/` | `UltimaSound.m` owns sound effects, speech, voice selection, music setup/update/end, and volume preferences. `LWSoundPlayer` wraps `NSSound`. `CocoaBridge.m` includes QuickTime sound-file playback helpers. |
| Preferences and dialogs | `Sources/PrefsDialog.m`, `Sources/PrefsDialog.h`, `Sources/PrefsDialogController.h`, `Sources/CocoaBridge.m`, `Sources/LWCocoaDialogController.h`, `Sources/LWIntegerTransformer.m`, `English.lproj/GameOptions.nib` | Preferences span Cocoa dialog controllers, KVO validation, value transformers, NIB loading, and legacy preference keys. Some display/prefs routines are also implemented in `UltimaNew.c` despite the file name. |
| Save/load | `Sources/UltimaMisc.c`, `Sources/UltimaMain.c`, `Sources/CocoaBridge.m` | Save/load is not yet fully mapped. `UltimaMisc.c` contains `GetParty`, `PutParty`, `GetRoster`, `PutRoster`, `GetSosaria`, `PutSosaria`, map load/push/pull/reset helpers, and resource opening. `UltimaMain.c` contains `QuitSave`. File/resource path support appears to involve `CocoaBridge.m`. |

## Source Responsibility Notes

- `Sources/UltimaMain.c` is the largest central module and mixes app startup, gameplay loops, command handling, state declarations, movement, stats, health, and UI waits.
- `Sources/UltimaMacIF.c` is the largest platform-facing module and mixes menus, windows, dialogs, cursor handling, display setup, macro input, alerts, image windows, and some character/stat UI.
- `Sources/UltimaMisc.c` is a broad gameplay/data module rather than a small utility file; it includes map access, resources, party/roster persistence, monsters, shops, shrines, moon gates, and several unnamed routines.
- `Sources/UltimaGraphics.c` centralizes most 2D drawing and image loading, but rendering is not isolated because dungeon, map, text, and UI modules draw directly too.
- `Sources/UltimaNew.c` is mixed-responsibility: it contains newer dialogs/preferences/display helpers, button drawing, auto-heal, stat clicks, window placement, and character-stat rendering.
- Small helper modules are comparatively bounded: `CarbonShunts.*`, `LWIntegerTransformer.*`, `LWSoundPlayer.*`, `UltimaAppleEvents.*`, and `main.m`.

## F-001A Inspection Notes

- Inspected file list with `rg --files Sources`.
- Reviewed headers for exported responsibilities.
- Reviewed line counts with `wc -l Sources/*`.
- Searched for function declarations/definitions, imports, globals, and Objective-C implementations with `rg`.
- This pass is responsibility-oriented; it does not prove runtime behavior or call ordering.

## Platform Dependencies

| Concern | Representative Files | Findings | Risk |
| --- | --- | --- | --- |
| UI and events | `Sources/UltimaMacIF.c`, `Sources/UltimaMain.c`, `Sources/UltimaNew.c`, `Sources/CocoaBridge.m`, `Sources/UltimaAppleEvents.c` | Uses Carbon/classic Mac event, menu, window, dialog, control, and Apple Event APIs such as `WaitNextEvent`, `EventRecord`, `WindowPtr`, `MenuHandle`, `DialogPtr`, `ControlHandle`, `FindWindow`, `MenuSelect`, `GetNewDialog`, `ModalDialog`, `AEInstallEventHandler`, and Cocoa `initWithWindowRef` wrapping. | High: Carbon Window/Menu/Dialog/Event Manager APIs are unsupported for modern 64-bit macOS. |
| Rendering | `Sources/UltimaGraphics.c`, `Sources/UltimaDngn.c`, `Sources/CarbonShunts.c` | Uses QuickDraw/GWorld/PICT drawing types and calls such as `CGrafPtr`, `GrafPtr`, `PixMapHandle`, `GDHandle`, `Rect`, `RGBColor`, `NewGWorld`, `LockPixels`, `CopyBits`, `CopyMask`, `PaintRect`, `FrameRect`, `ForeColor`, `BackColor`, `GetPicture`, and `DrawPicture`. Image import uses QuickTime Graphics Importer calls such as `GetGraphicsImporterForFile` and `GraphicsImportDraw`. | High: QuickDraw, GWorlds, PICT, and QuickTime Graphics Importer need replacement or an adapter layer. |
| Audio | `Sources/UltimaSound.m`, `Sources/CocoaBridge.m`, `Sources/LWSoundPlayer.m` | Main audio path imports QuickTime and uses movie APIs such as `EnterMovies`, `OpenMovieFile`, `NewMovieFromFile`, `SetMovieGWorld`, `SetMovieVolume`, `StartMovie`, `MoviesTask`, and `DisposeMovie`. Legacy sound resources use `GetResource` and `SndPlay`. `LWSoundPlayer` provides a smaller `NSSound` helper but is not the primary playback path. | High: QuickTime framework and Sound Manager APIs are unavailable on modern macOS. |
| Resources and files | `Resources/English.lproj/MainResources.rsrc`, `Ultima3.xcodeproj/project.pbxproj`, `Sources/UltimaMisc.c`, `Sources/UltimaGraphics.c`, `Sources/CocoaBridge.m` | Classic `.rsrc` data is processed by a Rez phase. Game data references resource types such as `MISC`, `MAPS`, `MONS`, `TLKS`, `PRTY`, and `ROST`. Save/roster persistence uses Resource Manager APIs such as `FSpOpenResFile`, `FSpCreateResFile`, `AddResource`, `ChangedResource`, and `WriteResource`. File paths use `FSSpec`, `FSRef`, `CFURLGetFSRef`, and `FSGetCatalogInfo`. Some bundle-based loading exists through `NSBundle`. | High: writable resource forks, `FSSpec`/`FSRef`, and Rez-based assets are major migration blockers. |
| Build settings | `Ultima3.xcodeproj/project.pbxproj` | Links Carbon and QuickTime frameworks. Debug architecture is `i386`; Release includes `ppc` and `i386`. Deployment targets include 10.4 and PPC 10.3.9. SDK settings reference `macosx10.6` and a local legacy PowerPC SDK path. Release scripts mention `Rez`, `hdiutil`, `osascript`, and old SDK paths. | High: 32-bit/PPC architectures, old SDKs, Carbon/QuickTime linking, and Rez build phases block Apple silicon builds. |
| Language and runtime assumptions | `Sources/UltimaText.h`, `Sources/UltimaAppleEvents.c`, `Sources/UltimaMisc.c`, `Sources/UltimaGraphics.c`, `Sources/CarbonShunts.c` | Uses `Str255` Pascal strings, `pascal` callback conventions, Carbon UPP handlers, Mac Memory Manager `Handle` APIs, pointer arithmetic through `long`, and conditional `TARGET_CARBON` compatibility shunts. | High: pointer-size assumptions, callback ABI, Pascal strings, and manual handle memory need isolation before 64-bit modernization. |

### F-001B Inspection Notes

- Explorer subagent performed read-only analysis.
- Search terms included `Carbon`, `QuickTime`, `WaitNextEvent`, `EventRecord`, `WindowPtr`, `GetNewDialog`, `NewGWorld`, `CopyBits`, `GraphicsImport`, `Movie`, `SndPlay`, `GetResource`, `AddResource`, `FSSpec`, `FSRef`, `SDKROOT`, `ARCHS`, `ppc`, `i386`, `Str255`, `Handle`, and `pascal`.
- This pass identifies representative dependencies and blockers; it is not a complete replacement design.

## State and Coupling Inventory

| Concern | Representative Files | Findings |
| --- | --- | --- |
| Primary shared runtime state | `Sources/UltimaMain.c` | `UltimaMain.c` is the primary owner of shared mutable runtime state. Representative globals include `Player[21][65]`, `Party[64]`, `Monsters[256]`, `Talk[256]`, `Dungeon[2048]`, `TileArray[128]`, `Macro[32]`, `zp[255]`, position registers such as `xpos`, `ypos`, `dx`, `dy`, `tx`, `ty`, map fields such as `gCurMapID`, `gCurMapSize`, `gMapOffset`, UI/input fields such as `gUpdateWhere`, `gMouseState`, `gKeyPress`, and many Carbon drawing/window handles. |
| Save/load and resource-backed state | `Sources/UltimaMisc.c` | Roster and party transfers, map/monster/talk loading, Sosaria push/pull, map accessors, and resource-file persistence are concentrated here. Relevant functions include `GetRoster`, `PutRoster`, `GetParty`, `PutParty`, `LoadUltimaMap`, `GetSosaria`, `PutSosaria`, `PushSosaria`, `PullSosaria`, `GetXYVal`, and `PutXYVal`. |
| Audio state | `Sources/UltimaSound.m` | Audio globals include song state (`gSongCurrent`, `gSongNext`, `gSongPlaying`), sample channels, sound handles, speech state, voice tables, QuickTime movie state, and the movie port/device used for music playback. |
| Mac interface state | `Sources/UltimaMacIF.c` | UI/platform globals include stats/dialog/display state such as `gStatsActive`, `gUnusualSize`, `gGrayRgn`, `chStatsCur`, current weapon/armour lists, and window/menu/cursor/display state. |
| Extern-based coupling | Most `.c` and `.m` subsystem files | Subsystems redeclare shared globals with local `extern` blocks instead of accessing state through a central interface. Heavy consumers include `UltimaMacIF.c`, `UltimaGraphics.c`, `UltimaMisc.c`, `UltimaSpellCombat.c`, `UltimaDngn.c`, and `UltimaAutocombat.c`. |
| Broad include coupling | `Sources/UltimaMain.c`, `Sources/UltimaGraphics.c`, `Sources/UltimaMacIF.c`, `Sources/UltimaMisc.c`, `Sources/UltimaDngn.c`, `Sources/UltimaSpellCombat.c`, `Sources/UltimaText.c` | High-traffic files include many subsystem headers, commonly combining `UltimaIncludes.h`, `CarbonShunts.h`, `CocoaBridge.h`, `UltimaMain.h`, `UltimaMacIF.h`, `UltimaGraphics.h`, `UltimaMisc.h`, `UltimaSound.h`, and `UltimaText.h`. |

## Coupling Hotspots

- `Sources/UltimaMain.c`: game loop, command dispatch, state mutation, rendering calls, sound calls, event polling, save prompts, and platform startup are mixed.
- `Sources/UltimaMacIF.c`: platform UI code directly reads and writes game state such as `Party`, `Player`, `gMouseState`, and `gUpdateWhere`, and calls game save/quit behavior.
- `Sources/UltimaGraphics.c`: renderer directly inspects and mutates gameplay arrays such as `TileArray`, `Monsters`, `Party`, and `Player`, and also touches sound channels and QuickTime tasks.
- `Sources/UltimaMisc.c`: map/resource persistence, monster AI, shops/shrines, map mutation, drawing, sound, and preferences are intertwined.
- `Sources/UltimaSpellCombat.c` and `Sources/UltimaDngn.c`: combat and dungeon rules mutate global game state while directly invoking rendering, sound, text, and input helpers.

## State Boundary Candidates

- Likely portable game/core state: `Player`, `Party`, `Monsters`, `Dungeon`, `Talk`, map handles/data, `TileArray`, `Macro`, locations, moon tables, experience tables, weapon/armour use tables, map position fields, character/monster combat arrays, torch/time/moon state, whirlpool state, and `dungeonLevel`.
- Likely platform/app-shell state: `WindowPtr`, `CGrafPtr`, `PixMapHandle`, `GDHandle`, `RgnHandle`, `EventRecord`, `MenuHandle`, `DialogFilterProc`, QuickTime movie/audio objects, preference `CFStringRef`s, update regions, display sizing/depth, cursor/window/menu globals.
- Blurred boundary requiring characterization before extraction: `gUpdateWhere`, `gMouseState`, `gKeyPress`, `gDone`, `gPaused`, `gInterrupt`, `gSongCurrent`, `gSongNext`, `gCurFrame`, `blkSiz`, and `zp[255]`.

### F-001C Inspection Notes

- Explorer subagent performed read-only analysis.
- Search terms included `extern`, broad `#import` patterns, global declarations, shared headers, and high-traffic subsystem files.
- This pass identifies coupling risks and candidate state boundaries; it does not define the final state model.

## Combat Action Resolution Inventory

`Sources/UltimaSpellCombat.c` is the legacy reference for combat action resolution, but it is not one clean subsystem. It mixes spell selection, player attacks, projectile animation, monster damage, monster turn behavior, poison, pilfering, combat setup, victory, rendering, text, sound, input, preferences, resource loading, and global state restoration.

### Main Combat Functions

| Function | Responsibility | Primary state touched | Platform or side effects | Extraction notes |
| --- | --- | --- | --- | --- |
| `Combat` | Full combat setup and turn loop, including player commands, monster turns, victory checks, autocombat handoff, and return to map or dungeon. | `Party`, `Player`, `Monsters`, `TileArray`, `CharX/Y/Tile/Shape`, `MonsterX/Y/Tile/HP`, `g835D/E/F`, `gSongCurrent`, `gSongNext`, `gTimeNegate`, `gUpdateWhere`, `gMouseState`, `gChnum`, `gKeyPress`, `Macro`, `zp`, `cHide`. | Reads preferences, flushes events, polls input, animates and draws tiles, prints messages, plays sounds, obscures cursor, calls UI commands such as stats, volume, ready weapon, and prompt drawing. | Too broad for first extraction. Treat as orchestration to split after smaller action results are characterized. |
| `CombatAttack` | Resolves a player attack after choosing attack command and direction. Handles weapon text, projectile or melee targeting, dagger throwing, Exodus Castle weapon rule, hit chance, hit animation, damage roll, and `DamageMonster`. | `Party`, `Player`, `MonsterX/Y/Tile/HP`, `CharX/Y`, `gMonType`, `gMonVarType`, `gBallTileBackground`, `zp`, `dx`, `dy`. | Reads weapon strings, prints attack text and monster text, asks direction, plays weapon/hit sounds, draws projectile and hit effects. | Good second extraction target if direction and random values are supplied explicitly and output events model text, sound, projectile, hit, miss, and inventory changes. |
| `DamageMonster` | Applies damage to one combat monster, handles death, experience message, experience award, restoring tile under monster, clearing HP, and redraw. | `MonsterHP`, `MonsterTile`, `MonsterX/Y`, `Player` through `AddExp`, `Experience`, `gMonType`, `TileArray`. | Builds Pascal strings from resources, prints experience message, draws tiles. `AddExp` can also play the level-up sound and refresh character display. | Best first combat-resolution extraction target. Core can return death, experience award, tile restore, level-up, character-redraw, and message-id/value events. |
| `Projectile` | Direction-based spell projectile in combat and damage handoff. | `Party`, `CharX/Y`, `MonsterX/Y/Tile`, `gBallTileBackground`, `zp`, `dx`, `dy`. | Asks direction, flashes spell, animates shot, plays hit or failed sounds, shows hit. | Defer until `Shoot` and event modeling are ready. |
| `BigDeath` | Area spell damage against all live combat monsters with random skip chance. | `MonsterHP`, `MonsterX/Y/Tile`, `TileArray`, `gBallTileBackground`. | Flash, random checks, temporary tile mutation, drawing, hit sound, pause, `DamageMonster`. | Defer until `DamageMonster` and deterministic random event sequencing are established. |
| `Necorp` | Sets all live combat monsters to 5 HP with hit animation. | `MonsterHP`, `MonsterX/Y/Tile`, `TileArray`, `gBallTileBackground`. | Flash, temporary tile mutation, drawing, hit sound, pause. | Possible later spell-effect extraction after common hit-event modeling exists. |
| `Poison` | Randomly poisons a targeted living character. | `Player[rosNum][17]`, `Party`, `wx`. | Prints attack/target and poison messages, plays `Ouch`. | Small extractable side-effect helper once target and random value are explicit. |
| `Pilfer` | Randomly removes a carried weapon or armour item from the target. | `Player[rosNum][40..56]`, `Party`, `wx`. | Prints attack/target and pilfer messages, plays `Ouch`. | Small extractable side-effect helper; needs explicit random draws and equipment indices. |
| `FigureNewMonPosition` | Chooses a monster target and movement vector. | `CharX/Y`, `MonsterX/Y`, `Player`, `Party`, `gMonType`, `zp`, `dx`, `dy`, `TileArray`. | None directly beyond shared globals. | Already related to extracted autocombat forecast logic, but still mutates `zp`, `dx`, and `dy`; could become part of monster-turn core. |
| `Shoot` | Advances projectile through combat grid until out of bounds or monster hit. | `TileArray`, `MonsterX/Y/Tile/HP`, `CharX/Y/Tile`, `gBallTileBackground`, `zp`, `dx`, `dy`. | Draws projectile and pauses each step. | Needs event-return model for projectile path before extraction. |
| `HandleMove` | Moves a combat character one tile if passable. | `CharX/Y/Tile`, `CharShape`, `TileArray`, `xs`, `ys`, `dx`, `dy`. | Step, bump, text, tile mutation. | Existing `CombatValidMove` coverage helps, but movement side effects need separate characterization. |
| `Victory` | Restores post-combat context and draws map or dungeon. | `gTimeNegate`, `gSongCurrent`, `gSongNext`, `Party[3]`, `g835E/F`. | Victory text, sound, window refresh, dungeon or map draw. | Defer until combat orchestration boundary exists. |
| `GetScreen` | Loads combat screen fixture data from `CONS` resource. | `TileArray`, `MonsterX/Y/Tile/HP`, `CharX/Y/Tile/Shape`. | Classic Resource Manager load/release. | Resource-backed data should become explicit fixture input before portable combat setup. |

### Side-Effect Categories

- State mutations: party location mode, player status/HP/MP/equipment/experience, combat monster HP/position/tile-under, combat character position/tile-under/shape, tile grid bytes, global direction registers, `zp` scratch registers, combat music fields, time-negate fields, and UI update state.
- Random dependencies: spell success checks, teleport/relocation coordinates, heal amounts, area spell monster skip checks, monster count and starting HP, monster turn choices, monster hit chance and damage, player attack hit chance and damage, poison chance, and pilfer branch/item choice.
- Text dependencies: message IDs around spell failure, combat prompts, hit/miss, victory, attack target, poison, pilfer, and experience award strings; weapon and monster names from plist-backed string tables.
- Rendering dependencies: tile writes for characters, monsters, projectile balls, hit effects, map pauses, mini-map/dungeon drawing, inverse tile or character flashes, prompt and frame redraws.
- Audio dependencies: combat start/victory, failed spell, spell-specific sounds, step, bump, attack, shoot, hit, monster spell, weapon swishes, ouch.
- Input and preference dependencies: spell selection, direction selection, target character selection, combat command input, manual/autocombat preference, diagonal movement preference, and hit-effect preference in `ShowHit`.
- Resource dependencies: `GetScreen` reads `CONS` resources; attack text and experience output read plist-backed Pascal strings.

### Recommended Extraction Sequence

1. `DamageMonster`: characterize monster damage, death, tile restoration, and experience award as a pure state transition plus events.
2. `CombatAttack`: after direction is supplied by the caller, characterize player attack targeting, hit/miss, dagger inventory mutation, Exodus Castle restriction, damage roll, and handoff to `DamageMonster`.
3. Monster turn side effects: characterize `Poison`, `Pilfer`, target/hit/miss/damage behavior, shooting, spell targeting, and movement as a bounded monster action result.

Do not extract `Combat()` as the next step. Its current coupling would force adapters for input, drawing, sound, preferences, resources, autocombat, and UI commands before the combat state model is stable.

### F-009 Inspection Notes

- Inspected `Sources/UltimaSpellCombat.c`, `Sources/UltimaSpellCombat.h`, `Sources/UltimaIncludes.h`, existing feature docs, and modernization strategy docs.
- Function search included `Combat`, `CombatAttack`, `DamageMonster`, `Projectile`, `BigDeath`, `Necorp`, `Poison`, `Pilfer`, `FigureNewMonPosition`, `Shoot`, `HandleMove`, `Victory`, and `GetScreen`.
- Side-effect search terms included `UPrint`, `PlaySoundFile`, `Draw`, `Inverse`, `GetKey`, `GetChar`, `GetDirection`, `RandNum`, `CFPreferences`, `GetResource`, `PutXY`, `GetXY`, `Show`, `HPAdd`, `HPSubtract`, `AddExp`, `CheckAlive`, `ReadyWeapon`, `NegateTime`, `Stats`, `Volume`, `What2`, `ScrollThings`, `AnimateTiles`, `FlushEvents`, `ObscureCursor`, `AutoCombat`, `AgeChars`, `Damage`, `Poison`, and `Pilfer`.
- This pass is an inventory and sequencing recommendation; it does not prove exact combat behavior or replace harness characterization.

## Data and Asset Formats

| Asset/Data Area | Paths / References | Notes |
| --- | --- | --- |
| Classic resource data | `Resources/English.lproj/MainResources.rsrc`, `Ultima3.xcodeproj/project.pbxproj`, `Sources/UltimaMisc.c`, `Sources/UltimaSound.m`, `Sources/UltimaSpellCombat.c`, `Sources/UltimaGraphics.c` | Main legacy data bundle is a `.rsrc` processed by a Rez build phase. Resource types observed include `MISC`, `MAPS`, `MONS`, `TLKS`, `PRTY`, `ROST`, `DEMO`, `CONS`, `SGNT`, `snd `, and PICT resources. |
| Localized strings | `Resources/English.lproj/Strings/*.plist`, `Sources/CocoaBridge.m`, `Sources/UltimaText.c` | Many old `GetIndString` calls are replaced or accompanied by plist-backed lookup through `StringsArray()` and `GetPascalStringFromArrayByIndex()`. String categories include messages, spells, races, classes, tiles, voices, weapons/armour, pub text, and gendered names. |
| Graphics sets | `Resources/Graphics/`, `Images/`, `Sources/UltimaGraphics.c` | Modern bundled graphics are PNG/GIF/JPEG files grouped by style, such as Standard, Apple II, Commodore 64, Nintendo, PC CGA/EGA/MCGA/VGA, and Macintosh B&W. `Images/` also includes menu, intro, dungeon, portrait, logo, and reference images. |
| Cursors | `Cursors/*.png`, `Cursors/CursorUse.txt`, `Sources/CocoaBridge.m`, `Sources/UltimaMacIF.c` | Cursor assets are PNG files with standard and `4X` variants. Cursor handling still crosses Cocoa bridge and legacy cursor state. |
| Sounds | `Resources/Sounds/*.wav`, `Resources/Sounds/*.mp3`, `Sources/UltimaSound.m`, `Sources/CocoaBridge.m` | Sound effects are mostly WAV with one MP3 observed. Playback paths include QuickTime movie loading and a smaller `NSSound` helper. |
| Music | `Resources/Music/Song_*.mov`, `Sources/UltimaSound.m` | Music is stored as `.mov` files and played through QuickTime movie APIs. |
| NIB/UI metadata | `English.lproj/GameOptions.nib`, `English.lproj/InfoPlist.strings`, `Sources/PrefsDialog.m`, `Sources/CocoaBridge.m` | Game options are loaded through a NIB and Cocoa controller classes. Info plist strings are top-level localized metadata. |
| Save/roster data | `Sources/UltimaMisc.c`, `Sources/UltimaMain.c` | Save-like state appears to be stored in Resource Manager files using `PRTY`, `ROST`, `MAPS`, `MONS`, and duplicated `MISC` resources. Exact file locations and compatibility requirements need deeper analysis. |

### Data/Asset Inspection Notes

- Inspected asset files with `find Resources English.lproj Images Cursors -maxdepth 4 -type f`.
- Searched source and project metadata for resource/file APIs, NIBs, strings, sounds, and graphics path helpers.
- This pass identifies formats and likely consumers; it does not define conversion strategy.

## Save and Resource Format Inventory

F-010 completed the first focused inventory of save and resource formats. The detailed feature record is `docs/features/F-010.md`.

### Save Container

The legacy save container is a writable Resource Manager file named `Ultima III Roster`, opened or created by `OpenRstr` in `Sources/UltimaMisc.c`. The file is located through the classic Preferences folder API and opened with `FSpOpenResFile` in read/write mode. First-run creation uses `FSpCreateResFile` with creator `Ult3` and type `RSTR`, then copies most default resources from the bundled resource data into mutable save resources. Current Sosaria `MONS` is an exception in the new-file path: it is created as a zero-filled 256-byte resource.

The roster file is not only a character roster. It owns active party bytes, roster records, current Sosaria map and monster state, mutable gameplay tables, and a legacy `PREF` resource.

| Mutable resource | Runtime state | Notes |
| --- | --- | --- |
| `PRTY` `BASERES` | `Party[1..64]` | Manual save updates `Party[4]`/`Party[5]` from `xpos`/`ypos`; `PutParty` writes only while `Party[3] == 0`. |
| `ROST` `BASERES` | `Player[1..20][0..63]` | 20 records of 64 bytes each. |
| `MAPS` `BASERES + 19` | Current Sosaria map | Size byte, map bytes, and whirlpool bytes. |
| `MONS` `BASERES + 19` | Current Sosaria monsters | 256 bytes. |
| `MISC` `BASERES + 100..105` | Mutable table copies | Moongates, type initial table, weapon-use table, armour-use table, location table, and experience table. `ResetSosaria` only resets `+100` and `+104` from defaults. |
| `PREF` `BASERES` | Legacy roster-file preferences | Created as 32 bytes with the first eight bytes set; no direct reads were found in this inventory pass. |

### Bundled Resource Data

`Resources/English.lproj/MainResources.rsrc` remains runtime-critical. The Xcode project includes it in a `PBXRezBuildPhase`; modern build/resource folders do not yet replace all Resource Manager reads.

Observed read-only resource families include `MAPS`, `MONS`, `TLKS`, `MISC`, `PRTY`, `ROST`, `CONS`, `DEMO`, `SGNT`, `PICT`, `snd `, legacy string lists, dialogs, alerts, and default save templates. `DeRez Resources/English.lproj/MainResources.rsrc` failed locally with `eofErr (-39)`, so complete binary enumeration remains a follow-up task.

### Save Flow Risks

- Manual `QuitSave` refuses to save unless the party is outdoors in Sosaria.
- Auto-save and map-transition flows call `PutRoster`, `PutParty`, `PutSosaria`, `PushSosaria`, and `PullSosaria` at specific times; these timings are part of compatibility.
- `LoadUltimaMap` performs an in-place migration of a 4100-byte current Sosaria `MAPS` resource to a 4101-byte shape with an inserted size byte.
- A modern persistence layer should be designed as an adapter over this resource-container behavior before changing save formats.

## Open Questions

- Which legacy resource types are still required at runtime after plist/PNG/WAV assets were introduced?
- Which save/roster data format must remain compatible in the modern app?
- Are `Images/` files source assets, build inputs, documentation/reference material, or a mix?
- Should music remain converted from `.mov`, or should the modern app adopt an audio-only format?
