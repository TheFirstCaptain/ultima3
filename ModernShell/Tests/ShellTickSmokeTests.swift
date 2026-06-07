import XCTest
@testable import Ultima3ModernShell

final class ShellTickSmokeTests: XCTestCase {
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
