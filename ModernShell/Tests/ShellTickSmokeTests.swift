import XCTest
import Ultima3Core
@testable import Ultima3ModernShell

final class ShellTickSmokeTests: XCTestCase {
    private let smokeGridWidth = 11
    private let partyTileValue: UInt16 = 0xff
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

        XCTAssertEqual(state.lastCommand, "Move East 43,20")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 1 audio 0 running")
        XCTAssertTrue(state.resourceStatus.contains("root "))
        let movedPartyCommand = renderCommand(in: state.renderFrame, index: partyCommandIndex)
        XCTAssertEqual(movedPartyCommand.value, partyTileValue)
    }

    func testTickMapsLegacyNumericKeyboardMovement() {
        let state = ShellSmokeState()

        state.submitKeyboard(UInt8(ascii: "6"))
        XCTAssertEqual(state.lastCommand, "Queued keyboard 6")

        state.runTick()

        XCTAssertEqual(state.lastCommand, "Move East 43,20")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 1 audio 0 running")
    }

    func testTickUsesWrappedSaveBackedOverworldMovement() {
        let state = ShellSmokeState()

        for _ in 0..<5 {
            state.submitOverworldCommand(moveWestCommand)
            state.runTick()
        }

        state.submitOverworldCommand(moveWestCommand)
        state.runTick()

        XCTAssertEqual(state.lastCommand, "Move West 36,20")
        XCTAssertEqual(state.tickStatus, "Tick 6 phase 2 input 6 audio 0 running")
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
    }

    func testSaveBackedOverworldMovementPersistsAfterWriteAndReload() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadNewGameSmoke()

        state.submitOverworldCommand(moveEastCommand)
        state.runTick()
        XCTAssertEqual(state.lastCommand, "Move East 43,20")

        state.writeSaveSmoke()
        XCTAssertEqual(state.lastCommand, "Save current")
        XCTAssertTrue(state.saveStatus.contains("Save OK Smoke | Save Read OK Smoke"))

        let reloadedState = ShellSmokeState(locationProvider: locationProvider)
        reloadedState.loadSavedSmoke()
        reloadedState.submitOverworldCommand(moveEastCommand)
        reloadedState.runTick()

        XCTAssertEqual(reloadedState.lastCommand, "Move East 44,20")
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

    func testWriteSaveSmokeRequiresCurrentDocument() {
        let state = ShellSmokeState()

        state.writeSaveSmoke()

        XCTAssertEqual(state.lastCommand, "Save current missing")
        XCTAssertEqual(state.saveStatus, "Save Missing current document")
    }

    func testLoadNewGameSmokeStoresCurrentDocumentForInspection() {
        let state = ShellSmokeState()

        state.loadNewGameSmoke()

        XCTAssertEqual(state.lastCommand, "Current save loaded")
        XCTAssertEqual(state.saveStatus, "Current Save OK party 64 roster 1280 map 4101 creatures 256 misc 16/11/11/11/64/16")

        state.inspectPartyRoster()

        XCTAssertEqual(state.lastCommand, "Party roster")
        XCTAssertEqual(state.saveStatus, "Party OK size 4 active 1/2/3/4 occupied 4 lead Tatiana G E/T/F HP 100/100 L1 food 150 gold 150")
    }

    func testAssembledPartyWritesAndLoadsFromSavedDocument() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadNewGameSmoke()
        state.acceptPartyAssembly([2, 1])

        state.writeSaveSmoke()
        XCTAssertEqual(state.lastCommand, "Save current")
        XCTAssertTrue(state.saveStatus.contains("Save OK Smoke | Save Read OK Smoke"))

        let reloadedState = ShellSmokeState(locationProvider: locationProvider)
        reloadedState.loadSavedSmoke()
        reloadedState.inspectPartyRoster()

        XCTAssertEqual(reloadedState.lastCommand, "Party roster")
        XCTAssertTrue(reloadedState.saveStatus.contains("Party OK size 2 active 2/1/0/0 occupied 4"))
    }

    func testStartupStyleLoadAdoptsSavedAssembledPartyWhenAvailable() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadNewGameSmoke()
        state.acceptPartyAssembly([2, 1])
        state.writeSaveSmoke()

        let launchedState = ShellSmokeState(locationProvider: locationProvider)
        launchedState.loadSavedSmokeIfAvailable()
        launchedState.inspectPartyRoster()

        XCTAssertEqual(launchedState.lastCommand, "Party roster")
        XCTAssertTrue(launchedState.saveStatus.contains("Party OK size 2 active 2/1/0/0 occupied 4"))
    }

    func testStartupStyleLoadNoOpsWhenSavedDocumentIsMissing() {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)

        state.loadSavedSmokeIfAvailable()

        XCTAssertEqual(state.lastCommand, "Ready")
        XCTAssertTrue(state.saveStatus.contains("Save path \(saveDocumentURL.path)"))
    }

    func testLoadSavedSmokeRejectsDomainInvalidDocumentWithoutReplacingCurrentDocument() throws {
        let locationProvider = ShellLocationProvider(saveDocumentURL: saveDocumentURL)
        let state = ShellSmokeState(locationProvider: locationProvider)
        state.loadNewGameSmoke()

        let invalidDomainDocument = makeStructurallyValidSaveDocument(payload: [1, 2, 3, 4])
        try FileManager.default.createDirectory(
            at: saveDocumentURL.deletingLastPathComponent(),
            withIntermediateDirectories: true
        )
        try invalidDomainDocument.write(to: saveDocumentURL)

        state.loadSavedSmoke()

        XCTAssertEqual(state.lastCommand, "Saved load failed")
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
}
