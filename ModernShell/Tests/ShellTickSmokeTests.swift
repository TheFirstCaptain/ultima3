import XCTest
import Ultima3Core
@testable import Ultima3ModernShell

final class ShellTickSmokeTests: XCTestCase {
    private let smokeGridWidth = 11
    private let partyTileValue: UInt16 = 0xff
    private let moveWestCommand: UInt16 = 3
    private let moveEastCommand: UInt16 = 4

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
        let initialPartyCommand = renderCommand(in: state.renderFrame, index: 2 + (5 * smokeGridWidth) + 5)
        XCTAssertEqual(initialPartyCommand.value, partyTileValue)

        state.submitOverworldCommand(moveEastCommand)
        XCTAssertEqual(state.lastCommand, "Queued move East")

        state.runTick()

        XCTAssertEqual(state.lastCommand, "Move East 6,5")
        XCTAssertEqual(state.tickStatus, "Tick 1 phase 1 input 1 audio 0 running")
        XCTAssertTrue(state.resourceStatus.contains("root "))
        let movedPartyCommand = renderCommand(in: state.renderFrame, index: 2 + (5 * smokeGridWidth) + 6)
        XCTAssertEqual(movedPartyCommand.value, partyTileValue)
    }

    func testTickReportsBlockedOverworldMovementAtSmokeBounds() {
        let state = ShellSmokeState()

        for _ in 0..<5 {
            state.submitOverworldCommand(moveWestCommand)
            state.runTick()
        }

        state.submitOverworldCommand(moveWestCommand)
        state.runTick()

        XCTAssertEqual(state.lastCommand, "Blocked West 0,5")
        XCTAssertEqual(state.tickStatus, "Tick 6 phase 2 input 6 audio 0 running")
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
}
