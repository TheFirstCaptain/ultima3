import XCTest
import Ultima3Core
@testable import Ultima3ModernShell

final class ShellTickSmokeTests: XCTestCase {
    private let smokeGridWidth = 11
    private let partyTileValue: UInt16 = 0xff
    private let moveNorthCommand: UInt16 = 1
    private let moveWestCommand: UInt16 = 3
    private let moveEastCommand: UInt16 = 4
    private var temporaryDirectory: URL!
    private var saveDocumentURL: URL!

    override func setUpWithError() throws {
        temporaryDirectory = FileManager.default.temporaryDirectory
            .appendingPathComponent("Ultima3ModernShellTests-\(UUID().uuidString)", isDirectory: true)
        saveDocumentURL = temporaryDirectory.appendingPathComponent("Ultima3ModernSave.u3save", isDirectory: false)
    }

    override func tearDownWithError() throws {
        if let temporaryDirectory {
            try? FileManager.default.removeItem(at: temporaryDirectory)
        }
    }

    func testTickAdvancesStatusWithoutQueuedEvents() {
        let state = ShellSmokeState()

        state.runTick()

        XCTAssertEqual(state.lastCommand, "Ready")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 0 audio 0 running")
    }

    func testTickConsumesQueuedKeyboardInput() {
        let state = ShellSmokeState()

        state.submitKeyboard(UInt8(ascii: "A"))
        XCTAssertEqual(state.lastCommand, "Queued keyboard A")

        state.runTick()

        XCTAssertEqual(state.lastCommand, "Keyboard A")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 1 audio 0 running")
    }

    func testTickAppliesQueuedOverworldMovementAndRefreshesFrame() {
        let state = ShellSmokeState()
        let partyCommandIndex = 2 + (5 * smokeGridWidth) + 5
        let initialPartyCommand = renderCommand(in: state.renderFrame, index: partyCommandIndex)
        XCTAssertEqual(initialPartyCommand.value, partyTileValue)

        state.submitOverworldCommand(moveEastCommand)
        XCTAssertEqual(state.lastCommand, "Queued move East")

        state.runTick()

        XCTAssertEqual(state.lastCommand, "Move East 43,20 | Audio Sound Step")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 1 audio 1 running")
        XCTAssertTrue(state.resourceStatus.contains("root "))
        let movedPartyCommand = renderCommand(in: state.renderFrame, index: partyCommandIndex)
        XCTAssertEqual(movedPartyCommand.value, partyTileValue)
    }

    func testTickMapsLegacyNumericKeyboardMovement() {
        let state = ShellSmokeState()

        state.submitKeyboard(UInt8(ascii: "6"))
        XCTAssertEqual(state.lastCommand, "Queued keyboard 6")

        state.runTick()

        XCTAssertEqual(state.lastCommand, "Move East 43,20 | Audio Sound Step")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 1 audio 1 running")
    }

    func testTickUsesWrappedSaveBackedOverworldMovement() {
        let state = ShellSmokeState()

        for _ in 0..<5 {
            state.submitOverworldCommand(moveWestCommand)
            state.runTick()
        }

        state.submitOverworldCommand(moveWestCommand)
        state.runTick()

        XCTAssertEqual(state.lastCommand, "Move West 36,20 | Audio Sound Step")
        XCTAssertEqual(state.tickStatus, "Tick 6 phase 2 input 6 audio 6 running")
    }

    func testTickReportsBlockedOverworldTerrain() {
        let state = ShellSmokeState()
        var blockedCommand: String?

        for _ in 0..<64 {
            state.submitOverworldCommand(moveEastCommand)
            state.runTick()
            if state.lastCommand.hasPrefix("Blocked East ") {
                blockedCommand = state.lastCommand
                break
            }
        }

        XCTAssertNotNil(blockedCommand)
        XCTAssertTrue(blockedCommand?.contains(" tile ") == true)
        XCTAssertTrue(blockedCommand?.contains("Audio Sound Bump") == true)
    }

    func testSaveBackedOverworldMovementPersistsAfterSaveAndReload() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.newGame()

        state.submitOverworldCommand(moveEastCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Move East 43,20 moves 4 | Audio Sound Step")

        XCTAssertTrue(state.hasUnsavedChanges)
        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Game saved")
        XCTAssertFalse(state.hasUnsavedChanges)
        XCTAssertTrue(state.saveStatus.contains("Game saved |"))

        let reloadedState = ShellSmokeState(locationProvider: locationProvider)
        reloadedState.loadGame()
        reloadedState.submitOverworldCommand(moveEastCommand)
        reloadedState.runTick()

        XCTAssertEqual(reloadedState.lastCommand, "Move East 44,20 moves 8 | Audio Sound Step")
    }

    func testGameplayMutationRemainsInMemoryUntilManualSave() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.newGame()
        state.saveGame()

        state.submitOverworldCommand(moveEastCommand)
        state.runTick()
        XCTAssertTrue(state.hasUnsavedChanges)

        let reloadedState = ShellSmokeState(locationProvider: locationProvider)
        reloadedState.loadGame()
        reloadedState.submitOverworldCommand(moveEastCommand)
        reloadedState.runTick()

        XCTAssertEqual(reloadedState.lastCommand, "Move East 43,20 moves 4 | Audio Sound Step")
    }

    func testManualSaveRejectsNonSosariaStateAndPreservesPriorSave() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.newGame()
        state.saveGame()
        let savedDocument = try Data(contentsOf: saveDocumentURL)

        var unsafeDocument = savedDocument
        XCTAssertTrue(setPartyLocation(1, in: &unsafeDocument))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(unsafeDocument, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        state.loadGame()
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(savedDocument, saveDocumentPath: saveDocumentURL.path),
            .saved
        )

        state.saveGame()

        XCTAssertEqual(state.lastCommand, "Save rejected")
        XCTAssertEqual(state.saveStatus, "Save unavailable outside Sosaria")
        XCTAssertEqual(try Data(contentsOf: saveDocumentURL), savedDocument)
    }

    func testSaveBackedBlockedOverworldMovementConsumesTurn() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        var blockedCommand: String?
        state.newGame()

        for _ in 0..<64 {
            state.submitOverworldCommand(moveEastCommand)
            state.runTick()
            if state.lastCommand.hasPrefix("Blocked East ") {
                blockedCommand = state.lastCommand
                break
            }
        }

        XCTAssertNotNil(blockedCommand)
        XCTAssertTrue(blockedCommand?.contains(" moves ") == true)
        XCTAssertTrue(blockedCommand?.contains("Audio Sound Bump") == true)
    }

    func testTownEntryMarksDocumentDirtyAndRejectsSave() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 46, y: 19, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let savedDocument = try Data(contentsOf: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadGame()

        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        XCTAssertEqual(
            state.lastCommand,
            "Entered town index 2 MAPS 402 return 46,19 start 1,32 heading 2 moves 4"
        )
        XCTAssertTrue(state.resourceStatus.contains("Location OK MAPS 402 kind 1 pos 1,32"))
        XCTAssertTrue(state.hasUnsavedChanges)

        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Save rejected")
        XCTAssertEqual(state.saveStatus, "Save unavailable outside Sosaria")
        XCTAssertEqual(try Data(contentsOf: saveDocumentURL), savedDocument)
        XCTAssertEqual(
            ShellTerminationPolicy.decision(
                action: .saveAndQuit,
                hasUnsavedChangesAfterSave: state.hasUnsavedChanges
            ),
            .cancel
        )
        XCTAssertEqual(
            ShellTerminationPolicy.decision(
                action: .quitWithoutSaving,
                hasUnsavedChangesAfterSave: state.hasUnsavedChanges
            ),
            .terminate
        )

        state.submitOverworldCommand(moveEastCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Location move East 2,32 moves 8 | Audio Sound Step")
        XCTAssertTrue(state.hasUnsavedChanges)

        state.refreshLocationStatus()
        XCTAssertEqual(state.lastCommand, "Location refreshed")
        XCTAssertTrue(state.resourceStatus.contains("Location OK MAPS 402 kind 1 pos 2,32"))
        XCTAssertTrue(state.hasUnsavedChanges)
    }

    func testTownTalkRoutesDirectionAndDoesNotConsumeTurn() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 46, y: 19, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadGame()
        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        state.submitKeyboard(UInt8(ascii: "T"))
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Talk: choose direction")
        state.submitOverworldCommand(moveEastCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Talk: no one at 2,32")

        state.submitOverworldCommand(moveWestCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Returned to Sosaria 46,19 moves 8 | Audio Sound Step")
        state.saveGame()
        let updatedDocument = try Data(contentsOf: saveDocumentURL)
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.moves, 8)
    }

    func testDungeonDoomEntryInitializesTransientSessionAndRejectsSave() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 19, y: 57, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let savedDocument = try Data(contentsOf: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadGame()

        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        XCTAssertEqual(
            state.lastCommand,
            "Entered dungeon index 12 MAPS 412 return 19,57 level 0 start 1,1 heading 1 moves 4"
        )
        XCTAssertTrue(state.resourceStatus.contains("Dungeon OK MAPS 412 level 0 pos 1,1 heading 1"))
        XCTAssertTrue(state.hasUnsavedChanges)

        state.submitOverworldCommand(moveEastCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Dungeon turn Right pos 1,1 heading 2 level 0 moves 8 | Audio Sound Step")
        state.refreshLocationStatus()
        XCTAssertEqual(state.lastCommand, "Location refreshed")
        XCTAssertTrue(state.resourceStatus.contains("Dungeon OK MAPS 412 level 0 pos 1,1 heading 2"))

        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Save rejected")
        XCTAssertEqual(state.saveStatus, "Save unavailable outside Sosaria")
        XCTAssertEqual(try Data(contentsOf: saveDocumentURL), savedDocument)
    }

    func testDungeonCommandsConsumeTurnsAndReportBlockedOrInvalidResults() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 19, y: 57, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadGame()
        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        state.submitKeyboard(UInt8(ascii: "D"))
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Dungeon invalid Descend pos 1,1 heading 1 level 0 moves 8")

        var blockedCommand: String?
        for _ in 0..<20 {
            state.submitOverworldCommand(moveNorthCommand)
            state.runTick()
            if state.lastCommand.hasPrefix("Dungeon blocked Forward ") {
                blockedCommand = state.lastCommand
                break
            }
        }

        XCTAssertNotNil(blockedCommand)
        XCTAssertTrue(blockedCommand?.contains("Audio Sound Bump") == true)
        XCTAssertTrue(blockedCommand?.contains(" moves ") == true)
    }

    func testFailedDungeonExitRetainsDungeonSessionAndDocument() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 19, y: 57, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let state = ShellSmokeState(
            locationProvider: locationProvider,
            locationTransitionAdapter: RejectingLocationTransitionAdapter(rejectExit: true)
        )
        state.loadGame()
        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        state.submitKeyboard(UInt8(ascii: "K"))
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Location transition failed")
        XCTAssertTrue(state.resourceStatus.contains("Dungeon OK MAPS 412 level 0 pos 1,1 heading 1"))

        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Save rejected")
    }

    func testDungeonKlimbRestoresExactSosariaSessionAndCanSave() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 19, y: 57, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadGame()
        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        state.submitKeyboard(UInt8(ascii: "K"))
        state.runTick()

        XCTAssertEqual(state.lastCommand, "Returned to Sosaria 19,57 moves 8 | Audio Sound Step")
        XCTAssertTrue(state.resourceStatus.contains("Overworld OK MAPS 419 pos 19,57"))
        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()
        XCTAssertEqual(
            state.lastCommand,
            "Entered dungeon index 12 MAPS 412 return 19,57 level 0 start 1,1 heading 1 moves 12"
        )
        state.submitKeyboard(UInt8(ascii: "K"))
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Returned to Sosaria 19,57 moves 16 | Audio Sound Step")

        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Game saved")
        let updatedDocument = try Data(contentsOf: saveDocumentURL)
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.location, 0)
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.x, 19)
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.y, 57)
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.moves, 16)
    }

    func testFailedDungeonEntryRetainsOutdoorDocumentAndFrame() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 19, y: 57, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let state = ShellSmokeState(
            locationProvider: locationProvider,
            locationTransitionAdapter: RejectingLocationTransitionAdapter(rejectEntry: true)
        )
        state.loadGame()

        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        XCTAssertEqual(state.lastCommand, "Location entry failed: invalid current state")
        XCTAssertFalse(state.hasUnsavedChanges)
        XCTAssertTrue(state.resourceStatus.contains("Overworld OK MAPS 419 pos 19,57"))
        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Game saved")
    }

    func testFailedTownEntryRetainsOutdoorDocumentAndFrame() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 46, y: 19, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let state = ShellSmokeState(
            locationProvider: locationProvider,
            locationTransitionAdapter: RejectingLocationTransitionAdapter(rejectEntry: true)
        )
        state.loadGame()

        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        XCTAssertEqual(state.lastCommand, "Location entry failed: invalid current state")
        XCTAssertFalse(state.hasUnsavedChanges)
        XCTAssertTrue(state.resourceStatus.contains("Overworld OK MAPS 419 pos 46,19"))
        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Game saved")
    }

    func testFailedTownExitRetainsTownSessionAndDocument() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 46, y: 19, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let state = ShellSmokeState(
            locationProvider: locationProvider,
            locationTransitionAdapter: RejectingLocationTransitionAdapter(rejectExit: true)
        )
        state.loadGame()
        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        state.submitOverworldCommand(moveWestCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Location transition failed")
        XCTAssertTrue(state.resourceStatus.contains("Location OK MAPS 402 kind 1 pos 1,32"))

        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Save rejected")
        state.submitOverworldCommand(moveEastCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Location move East 2,32 moves 8 | Audio Sound Step")
    }

    func testTownWestBoundaryRestoresExactSosariaSessionAndCanSave() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setPartyPosition(x: 46, y: 19, in: &document))
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(document, saveDocumentPath: saveDocumentURL.path),
            .saved
        )
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadGame()
        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        state.submitOverworldCommand(moveEastCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Location move East 2,32 moves 8 | Audio Sound Step")
        state.submitOverworldCommand(moveWestCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Location move West 1,32 moves 12 | Audio Sound Step")

        state.submitOverworldCommand(moveWestCommand)
        state.runTick()

        XCTAssertEqual(state.lastCommand, "Returned to Sosaria 46,19 moves 16 | Audio Sound Step")
        XCTAssertTrue(state.resourceStatus.contains("Overworld OK MAPS 419 pos 46,19"))
        XCTAssertTrue(state.hasUnsavedChanges)

        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Game saved")
        let updatedDocument = try Data(contentsOf: saveDocumentURL)
        XCTAssertEqual(
            saveRecordData(type: 0x4D415053, id: Int16(U3_SAVE_ID_CURRENT_SOSARIA), in: updatedDocument),
            saveRecordData(type: 0x4D415053, id: Int16(U3_SAVE_ID_CURRENT_SOSARIA), in: document)
        )
        XCTAssertEqual(
            saveRecordData(type: UInt32(U3_SAVE_TYPE_CREATURES), id: Int16(U3_SAVE_ID_CURRENT_SOSARIA), in: updatedDocument),
            saveRecordData(type: UInt32(U3_SAVE_TYPE_CREATURES), id: Int16(U3_SAVE_ID_CURRENT_SOSARIA), in: document)
        )
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.location, 0)
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.x, 46)
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.y, 19)
        XCTAssertEqual(partyPositionAndMoves(in: updatedDocument)?.moves, 16)

        let reloadedState = ShellSmokeState(locationProvider: locationProvider)
        reloadedState.loadGame()
        XCTAssertTrue(reloadedState.resourceStatus.contains("Overworld OK MAPS 419 pos 46,19"))
    }

    func testEnterAtUnrecognizedCoordinateReportsUnavailable() {
        let state = ShellSmokeState()
        state.newGame()

        state.submitKeyboard(UInt8(ascii: "E"))
        state.runTick()

        XCTAssertEqual(state.lastCommand, "Enter unavailable 42,20")
        XCTAssertTrue(state.hasUnsavedChanges)
    }

    private func renderCommand(in frame: u3_render_frame, index: Int) -> u3_render_command {
        var frame = frame
        return withUnsafePointer(to: &frame.commands) { pointer in
            pointer.withMemoryRebound(to: u3_render_command.self, capacity: 160) { commands in
                commands[index]
            }
        }
    }

    func testTickDispatchesQueuedAudio() {
        let state = ShellSmokeState()

        state.submitAudioSound(1)
        XCTAssertEqual(state.lastCommand, "Queued audio 1")

        state.runTick()

        XCTAssertTrue(state.lastCommand == "Audio Sound Error1" || state.lastCommand == "Audio Missing Error1" || state.lastCommand == "Audio Failed Error1")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 0 audio 1 running")
    }

    func testTickPauseStatusDoesNotAdvanceCoreTick() {
        let state = ShellSmokeState()

        state.runTick()
        state.setTickActive(false)

        XCTAssertEqual(state.lastCommand, "Tick paused")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 0 audio 0 paused")
    }

    func testSaveGameRequiresCurrentDocument() {
        let state = ShellSmokeState()

        state.saveGame()

        XCTAssertEqual(state.lastCommand, "Save unavailable")
        XCTAssertEqual(state.saveStatus, "Save unavailable: start or load a game first")
    }

    func testNewGameStoresCurrentDocumentForInspection() {
        let state = ShellSmokeState()

        state.newGame()

        XCTAssertEqual(state.lastCommand, "New game started")
        XCTAssertTrue(state.hasUnsavedChanges)
        XCTAssertEqual(state.saveStatus, "Current Save OK party 64 roster 1280 map 4101 creatures 256 misc 16/11/11/11/64/16")

        state.inspectPartyRoster()

        XCTAssertEqual(state.lastCommand, "Party roster")
        XCTAssertEqual(state.saveStatus, "Party OK size 4 active 1/2/3/4 occupied 4 lead Tatiana G E/T/F HP 100/100 L1 food 150 gold 150")
    }

    func testAssembledPartyWritesAndLoadsFromSavedDocument() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.newGame()
        state.acceptPartyAssembly([2, 1])

        state.saveGame()
        XCTAssertEqual(state.lastCommand, "Game saved")
        XCTAssertTrue(state.saveStatus.contains("Game saved |"))

        let reloadedState = ShellSmokeState(locationProvider: locationProvider)
        reloadedState.loadGame()
        reloadedState.inspectPartyRoster()

        XCTAssertEqual(reloadedState.lastCommand, "Party roster")
        XCTAssertTrue(reloadedState.saveStatus.contains("Party OK size 2 active 2/1/0/0 occupied 4"))
    }

    func testStartupStyleLoadAdoptsSavedAssembledPartyWhenAvailable() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.newGame()
        state.acceptPartyAssembly([2, 1])
        state.saveGame()

        let launchedState = ShellSmokeState(locationProvider: locationProvider)
        launchedState.loadSavedGameIfAvailable()
        launchedState.inspectPartyRoster()

        XCTAssertEqual(launchedState.lastCommand, "Party roster")
        XCTAssertTrue(launchedState.saveStatus.contains("Party OK size 2 active 2/1/0/0 occupied 4"))
    }

    func testStartupStyleLoadNoOpsWhenSavedDocumentIsMissing() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)

        state.loadSavedGameIfAvailable()

        XCTAssertEqual(state.lastCommand, "Ready")
        XCTAssertTrue(state.saveStatus.contains("Save path \(saveDocumentURL.path)"))
    }

    func testLoadGameRejectsDomainInvalidDocumentWithoutReplacingCurrentDocument() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.newGame()

        let invalidDomainDocument = makeStructurallyValidSaveDocument(payload: [1, 2, 3, 4])
        try FileManager.default.createDirectory(
            at: saveDocumentURL.deletingLastPathComponent(),
            withIntermediateDirectories: true
        )
        try invalidDomainDocument.write(to: saveDocumentURL)

        state.loadGame()

        XCTAssertEqual(state.lastCommand, "Load failed")
        XCTAssertEqual(state.saveStatus, "Saved Game Failed load domain state")

        state.inspectPartyRoster()

        XCTAssertEqual(state.lastCommand, "Party roster")
        XCTAssertEqual(state.saveStatus, "Party OK size 4 active 1/2/3/4 occupied 4 lead Tatiana G E/T/F HP 100/100 L1 food 150 gold 150")
    }

    private func makeStructurallyValidSaveDocument(payload: [UInt8]) -> Data {
        var data = Data(count: 16 + 16)
        data[0] = UInt8(ascii: "U")
        data[1] = UInt8(ascii: "3")
        data[2] = UInt8(ascii: "S")
        data[3] = UInt8(ascii: "V")
        writeUInt16(1, at: 4, into: &data)
        writeUInt16(1, at: 6, into: &data)
        writeUInt32(32, at: 8, into: &data)
        writeUInt32(0x4D455441, at: 16, into: &data)
        writeUInt16(1, at: 20, into: &data)
        writeUInt16(0, at: 22, into: &data)
        writeUInt32(32, at: 24, into: &data)
        writeUInt32(UInt32(payload.count), at: 28, into: &data)
        data.append(contentsOf: payload)
        return data
    }

    private func writeUInt16(_ value: UInt16, at offset: Int, into data: inout Data) {
        data[offset] = UInt8(value >> 8)
        data[offset + 1] = UInt8(value & 0xFF)
    }

    private func writeUInt32(_ value: UInt32, at offset: Int, into data: inout Data) {
        data[offset] = UInt8(value >> 24)
        data[offset + 1] = UInt8((value >> 16) & 0xFF)
        data[offset + 2] = UInt8((value >> 8) & 0xFF)
        data[offset + 3] = UInt8(value & 0xFF)
    }

    private func setPartyLocation(_ location: UInt8, in documentData: inout Data) -> Bool {
        let documentLength = documentData.count
        return documentData.withUnsafeMutableBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }

            var document = u3_save_document()
            guard u3_save_open(documentBaseAddress, documentLength, &document) != 0 else {
                return false
            }

            var partyRecord = u3_save_record()
            guard u3_save_find_record(&document, 0x50525459, Int16(U3_SAVE_ID_PARTY), &partyRecord) != 0,
                  partyRecord.length == U3_SAVE_PARTY_LENGTH,
                  let party = UnsafeMutableRawPointer(mutating: partyRecord.data)?.assumingMemoryBound(to: UInt8.self) else {
                return false
            }

            party[2] = location
            return true
        }
    }

    private var resourceRootPath: String {
        URL(fileURLWithPath: #filePath)
            .deletingLastPathComponent()
            .deletingLastPathComponent()
            .deletingLastPathComponent()
            .appendingPathComponent("Resources", isDirectory: true)
            .path
    }

    private func setPartyPosition(x: UInt8, y: UInt8, in documentData: inout Data) -> Bool {
        let documentLength = documentData.count
        return documentData.withUnsafeMutableBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }

            var document = u3_save_document()
            guard u3_save_open(documentBaseAddress, documentLength, &document) != 0 else {
                return false
            }

            var partyRecord = u3_save_record()
            guard u3_save_find_record(&document, 0x50525459, Int16(U3_SAVE_ID_PARTY), &partyRecord) != 0,
                  partyRecord.length == U3_SAVE_PARTY_LENGTH,
                  let party = UnsafeMutableRawPointer(mutating: partyRecord.data)?.assumingMemoryBound(to: UInt8.self) else {
                return false
            }

            party[Int(U3_OVERWORLD_PARTY_X_OFFSET)] = x
            party[Int(U3_OVERWORLD_PARTY_Y_OFFSET)] = y
            return true
        }
    }

    private func partyPositionAndMoves(in documentData: Data) -> (location: UInt8, x: UInt8, y: UInt8, moves: UInt32)? {
        documentData.withUnsafeBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return nil
            }

            var document = u3_save_document()
            guard u3_save_open(documentBaseAddress, documentData.count, &document) != 0 else {
                return nil
            }

            var partyRecord = u3_save_record()
            guard u3_save_find_record(&document, 0x50525459, Int16(U3_SAVE_ID_PARTY), &partyRecord) != 0,
                  let party = partyRecord.data else {
                return nil
            }

            return (
                party[Int(U3_LOCATION_PARTY_MODE_OFFSET)],
                party[Int(U3_OVERWORLD_PARTY_X_OFFSET)],
                party[Int(U3_OVERWORLD_PARTY_Y_OFFSET)],
                u3_overworld_read_party_move_counter(party, partyRecord.length)
            )
        }
    }

    private func saveRecordData(type: UInt32, id: Int16, in documentData: Data) -> Data? {
        documentData.withUnsafeBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return nil
            }

            var document = u3_save_document()
            var record = u3_save_record()
            guard u3_save_open(documentBaseAddress, documentData.count, &document) != 0,
                  u3_save_find_record(&document, type, id, &record) != 0,
                  let bytes = record.data else {
                return nil
            }

            return Data(bytes: bytes, count: Int(record.length))
        }
    }
}

private final class RejectingLocationTransitionAdapter: ShellLocationDocumentTransitioning {
    private let wrapped = ShellLocationDocumentTransitionAdapter()
    private let rejectEntry: Bool
    private let rejectExit: Bool

    init(rejectEntry: Bool = false, rejectExit: Bool = false) {
        self.rejectEntry = rejectEntry
        self.rejectExit = rejectExit
    }

    func applyEntry(
        to documentData: inout Data,
        request: inout u3_location_transition_result
    ) -> Bool {
        if rejectEntry {
            return false
        }
        return wrapped.applyEntry(to: &documentData, request: &request)
    }

    func applyMove(
        to documentData: inout Data,
        session: ShellLocationSession,
        result: inout u3_location_move_result
    ) -> Bool {
        if rejectExit && result.exit_requested != 0 {
            return false
        }
        return wrapped.applyMove(to: &documentData, session: session, result: &result)
    }
}
