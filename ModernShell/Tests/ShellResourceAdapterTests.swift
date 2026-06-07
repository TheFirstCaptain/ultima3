import Foundation
import XCTest
@testable import Ultima3ModernShell

final class ShellResourceAdapterTests: XCTestCase {
    func testLoadNewGameSmokeStateReportsDomainLengths() {
        let adapter = ShellResourceAdapter()
        let resourceRootPath = URL(fileURLWithPath: FileManager.default.currentDirectoryPath, isDirectory: true)
            .appendingPathComponent("Resources", isDirectory: true)
            .path

        XCTAssertEqual(
            adapter.loadNewGameSmokeState(resourceRootPath: resourceRootPath),
            "New Game OK party 64 roster 1280 map 4101 creatures 256 misc 16/11/11/11/64/16"
        )
    }

    func testLoadNewGameSmokeStateReportsMissingInput() {
        let adapter = ShellResourceAdapter()

        XCTAssertEqual(
            adapter.loadNewGameSmokeState(resourceRootPath: nil),
            "New Game Failed build smoke document"
        )
    }
}
