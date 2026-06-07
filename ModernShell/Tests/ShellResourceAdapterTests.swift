import Foundation
import XCTest
@testable import Ultima3ModernShell

final class ShellResourceAdapterTests: XCTestCase {
    private let renderTileCount = 121
    private let renderCommandClear = 1
    private let renderCommandRect = 2
    private let renderCommandTile = 3

    private var resourceRootPath: String {
        URL(fileURLWithPath: FileManager.default.currentDirectoryPath, isDirectory: true)
            .appendingPathComponent("Resources", isDirectory: true)
            .path
    }

    func testLoadNewGameSmokeStateReportsDomainLengths() {
        let adapter = ShellResourceAdapter()

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

    func testBuildResourceBackedRenderSmokeFrameUsesCombatScreenTiles() {
        let adapter = ShellResourceAdapter()
        let result = adapter.buildResourceBackedRenderSmokeFrame(resourceRootPath: resourceRootPath)

        XCTAssertEqual(result.status, "Render OK CONS 400 tiles 121")
        XCTAssertEqual(result.frame.command_count, UInt16(renderTileCount + 2))
        XCTAssertEqual(Int(result.frame.commands.0.kind), renderCommandClear)
        XCTAssertEqual(Int(result.frame.commands.1.kind), renderCommandRect)
        XCTAssertEqual(Int(result.frame.commands.2.kind), renderCommandTile)
        XCTAssertEqual(result.frame.commands.2.value, 0x00)
        XCTAssertEqual(result.frame.commands.2.x, 1)
        XCTAssertEqual(result.frame.commands.2.y, 1)
        XCTAssertEqual(Int(result.frame.commands.3.kind), renderCommandTile)
        XCTAssertEqual(result.frame.commands.3.value, 0x46)
        XCTAssertEqual(result.frame.commands.3.x, 2)
        XCTAssertEqual(result.frame.commands.3.y, 1)
    }

    func testBuildResourceBackedRenderSmokeFrameFallsBackWhenMissingInput() {
        let adapter = ShellResourceAdapter()
        let result = adapter.buildResourceBackedRenderSmokeFrame(resourceRootPath: nil)

        XCTAssertEqual(result.status, "Render Missing resource root")
        XCTAssertEqual(result.frame.command_count, UInt16(renderTileCount + 2))
    }
}
