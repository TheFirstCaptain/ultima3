import Foundation
import Ultima3Core
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

    func testInspectPartyRosterReportsDefaultParty() {
        let adapter = ShellResourceAdapter()

        XCTAssertEqual(
            adapter.inspectPartyRoster(resourceRootPath: resourceRootPath),
            "Party OK size 4 active 1/2/3/4 occupied 4 lead Tatiana G E/T/F HP 100/100 L1 food 150 gold 150"
        )
    }

    func testFormatPartyRosterSummaryUsesActiveLeaderRosterID() {
        let adapter = ShellResourceAdapter()
        var summary = u3_party_summary()
        summary.party_size = 1
        summary.active_roster_ids.0 = 2
        summary.occupied_roster_count = 1
        summary.roster.1.roster_id = 2
        summary.roster.1.occupied = 1
        summary.roster.1.name = (78, 111, 114, 114, 105, 99, 0, 0, 0, 0, 0, 0, 0, 0)
        summary.roster.1.status = 71
        summary.roster.1.race = 68
        summary.roster.1.character_class = 70
        summary.roster.1.sex = 77
        summary.roster.1.hit_points = 88
        summary.roster.1.max_hit_points = 99
        summary.roster.1.level = 3
        summary.roster.1.food = 120
        summary.roster.1.gold = 345

        XCTAssertEqual(
            adapter.formatPartyRosterSummary(&summary),
            "Party OK size 1 active 2/0/0/0 occupied 1 lead Norric G D/F/M HP 88/99 L3 food 120 gold 345"
        )
    }

    func testInspectPartyRosterReportsMissingInput() {
        let adapter = ShellResourceAdapter()

        XCTAssertEqual(
            adapter.inspectPartyRoster(resourceRootPath: nil),
            "Party Failed build smoke document"
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
