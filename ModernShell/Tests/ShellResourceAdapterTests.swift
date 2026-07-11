import Foundation
import Ultima3Core
import XCTest
@testable import Ultima3ModernShell

final class ShellResourceAdapterTests: XCTestCase {
    private let renderTileCount = 121
    private let renderCommandClear = 1
    private let renderCommandRect = 2
    private let renderCommandTile = 3
    private let dungeonWallValue = Int(U3_DUNGEON_RENDER_VALUE_WALL)
    private let dungeonDoorValue = Int(U3_DUNGEON_RENDER_VALUE_DOOR)

    private var resourceRootPath: String {
        URL(fileURLWithPath: #filePath)
            .deletingLastPathComponent()
            .deletingLastPathComponent()
            .deletingLastPathComponent()
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

    func testDescribeSaveDomainReportsCurrentDocumentLengths() throws {
        let adapter = ShellResourceAdapter()
        let document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))

        XCTAssertEqual(
            adapter.describeSaveDomain(document, prefix: "Current Save"),
            "Current Save OK party 64 roster 1280 map 4101 creatures 256 misc 16/11/11/11/64/16"
        )
        XCTAssertTrue(adapter.currentSosariaSaveAllowed(documentData: document))
    }

    func testBuildNativeNewGameSmokeDocumentSeedsTorchInventory() throws {
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))

        let result = adapter.igniteTorch(documentData: &document)

        XCTAssertTrue(result.ignited != 0)
        XCTAssertEqual(result.roster_id, 1)
        XCTAssertEqual(result.torch_count_before, 2)
        XCTAssertEqual(result.torch_count_after, 1)
        XCTAssertEqual(result.light, UInt8(U3_PARTY_TORCH_LIGHT_VALUE))
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

    func testInspectPartyRosterReadsProvidedCurrentDocument() throws {
        let adapter = ShellResourceAdapter()
        let document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))

        XCTAssertEqual(
            adapter.inspectPartyRoster(documentData: document),
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

    func testLoadLocationSessionOwnsTownFixtureDataAndBuildsFrame() throws {
        let adapter = ShellResourceAdapter()
        let result = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_TOWN), index: 2, x: 1, y: 32, heading: 2)
        )

        guard case .success(let session) = result else {
            return XCTFail("Expected LCB Towne session")
        }
        XCTAssertEqual(session.descriptor.resource_id, 402)
        XCTAssertEqual(Int32(session.descriptor.map_shape), U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL)
        XCTAssertEqual(session.mapData.count, Int(U3_LOCATION_TWO_DIMENSIONAL_MAP_LENGTH))
        XCTAssertEqual(session.monsterData?.count, Int(U3_LOCATION_MONSTER_LENGTH))
        XCTAssertEqual(session.talkData.count, Int(U3_LOCATION_TALK_LENGTH))
        XCTAssertEqual(session.frame.command_count, UInt16(renderTileCount + 2))
        XCTAssertEqual(session.mapData.first, 64)
        XCTAssertEqual(session.status, "Location OK MAPS 402 kind 1 pos 1,32")
    }

    func testTownTalkAdapterDisplaysStableBundledGuardLine() {
        let adapter = ShellResourceAdapter()
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_TOWN), index: 2, x: 1, y: 32, heading: 2)
        )
        guard case .success(var session) = loadResult else {
            return XCTFail("Expected LCB Towne session")
        }
        session.descriptor.x = 26
        session.descriptor.y = 32
        var talkResult = u3_location_talk_result()

        XCTAssertTrue(adapter.talkLocationSession(
            session,
            direction: 4,
            result: &talkResult
        ))
        XCTAssertEqual(talkResult.npc_slot, 31)
        XCTAssertEqual(talkResult.talk_index, 8)
        XCTAssertEqual(adapter.describeLocationTalk(&talkResult), "Talk: HO! HO!")
    }

    func testLoadLocationSessionValidatesCastleAndDungeonFamilies() {
        let adapter = ShellResourceAdapter()
        let castleResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_CASTLE), index: 0, x: 32, y: 62, heading: 2)
        )
        guard case .success(let castle) = castleResult else {
            return XCTFail("Expected castle session")
        }
        XCTAssertEqual(castle.descriptor.resource_id, 400)
        XCTAssertEqual(castle.monsterData?.count, 256)

        let dungeonResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(let dungeon) = dungeonResult else {
            return XCTFail("Expected dungeon session")
        }
        XCTAssertEqual(dungeon.descriptor.resource_id, 412)
        XCTAssertEqual(Int32(dungeon.descriptor.map_shape), U3_LOCATION_MAP_SHAPE_DUNGEON)
        XCTAssertEqual(dungeon.mapData.count, 2048)
        XCTAssertNil(dungeon.monsterData)
        XCTAssertEqual(dungeon.talkData.count, 256)
        XCTAssertEqual(dungeon.descriptor.dungeon_level, 0)
        XCTAssertEqual(dungeon.descriptor.x, 1)
        XCTAssertEqual(dungeon.descriptor.y, 1)
        XCTAssertEqual(dungeon.descriptor.heading, 1)
        XCTAssertEqual(Int(dungeon.frame.commands.0.kind), renderCommandClear)
        XCTAssertEqual(Int(dungeon.frame.commands.1.kind), renderCommandRect)
        XCTAssertEqual(dungeon.frame.command_count, 2)
        XCTAssertEqual(countCommands(in: dungeon.frame, value: dungeonWallValue), 0)
        XCTAssertEqual(countCommands(in: dungeon.frame, value: dungeonDoorValue), 0)
        XCTAssertEqual(dungeon.status, "Dungeon OK MAPS 412 level 0 pos 1,1 heading 1 light 0")
    }

    func testMoveDungeonSessionMapsCommandsAndRefreshesFrame() {
        let adapter = ShellResourceAdapter()
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(var dungeon) = loadResult else {
            return XCTFail("Expected dungeon session")
        }
        dungeon.descriptor.light = UInt8(U3_DUNGEON_LIGHT_FULL)
        adapter.refreshLocationSessionFrame(&dungeon)
        var result = u3_dungeon_result()

        XCTAssertTrue(adapter.moveDungeonSession(&dungeon, command: 4, result: &result))
        XCTAssertTrue(result.turned)
        XCTAssertTrue(result.needs_redraw)
        XCTAssertEqual(dungeon.descriptor.heading, 2)
        XCTAssertEqual(dungeon.status, "Dungeon OK MAPS 412 level 0 pos 1,1 heading 2 light 255")
        XCTAssertGreaterThan(dungeon.frame.command_count, 2)

        XCTAssertTrue(adapter.moveDungeonSession(&dungeon, command: UInt16(UInt8(ascii: "D")), result: &result))
        XCTAssertTrue(result.invalid)
        XCTAssertEqual(dungeon.descriptor.dungeon_level, 0)

        XCTAssertFalse(adapter.moveDungeonSession(&dungeon, command: UInt16(UInt8(ascii: "E")), result: &result))
    }

    func testMoveDungeonSessionValidatesShapeAndMapBoundsBeforeNavigation() {
        let adapter = ShellResourceAdapter()
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(let dungeon) = loadResult else {
            return XCTFail("Expected dungeon session")
        }
        var truncated = ShellLocationSession(
            descriptor: dungeon.descriptor,
            mapData: Data(dungeon.mapData.prefix(16)),
            monsterData: nil,
            talkData: dungeon.talkData,
            frame: dungeon.frame
        )
        var outOfBounds = dungeon
        outOfBounds.descriptor.dungeon_level = UInt8(U3_DUNGEON_LEVEL_COUNT)
        var result = u3_dungeon_result()

        XCTAssertFalse(adapter.moveDungeonSession(&truncated, command: 4, result: &result))
        XCTAssertFalse(adapter.moveDungeonSession(&outOfBounds, command: 4, result: &result))
    }

    func testMoveDungeonSessionSupportsValidLevelChangesWithoutExiting() {
        let adapter = ShellResourceAdapter()
        var descriptor = u3_location_session()
        descriptor.active = 1
        descriptor.destination_kind = UInt8(U3_LOCATION_KIND_DUNGEON)
        descriptor.location_index = UInt8(U3_LOCATION_DUNGEON_FIXTURE_INDEX)
        descriptor.resource_id = UInt16(U3_LOCATION_DUNGEON_FIXTURE_RESOURCE_ID)
        descriptor.map_shape = UInt8(U3_LOCATION_MAP_SHAPE_DUNGEON)
        descriptor.map_size = UInt16(U3_DUNGEON_WIDTH)
        descriptor.map_length = UInt32(U3_LOCATION_DUNGEON_MAP_LENGTH)
        descriptor.talk_length = UInt32(U3_LOCATION_TALK_LENGTH)
        descriptor.x = 5
        descriptor.y = 5
        descriptor.heading = 1
        descriptor.dungeon_level = 0
        var mapData = Data(count: Int(U3_LOCATION_DUNGEON_MAP_LENGTH))
        mapData[(5 * Int(U3_DUNGEON_WIDTH)) + 5] = UInt8(U3_DUNGEON_TILE_DOWN_LADDER)
        mapData[(Int(U3_DUNGEON_WIDTH) * Int(U3_DUNGEON_HEIGHT)) + (5 * Int(U3_DUNGEON_WIDTH)) + 5] = UInt8(U3_DUNGEON_TILE_UP_LADDER)
        var session = ShellLocationSession(
            descriptor: descriptor,
            mapData: mapData,
            monsterData: nil,
            talkData: Data(count: Int(U3_LOCATION_TALK_LENGTH)),
            frame: u3_render_make_synthetic_tile_frame()
        )
        var result = u3_dungeon_result()

        XCTAssertTrue(adapter.moveDungeonSession(&session, command: UInt16(UInt8(ascii: "D")), result: &result))
        XCTAssertTrue(result.level_changed)
        XCTAssertTrue(result.needs_redraw)
        XCTAssertFalse(result.exited)
        XCTAssertEqual(session.descriptor.dungeon_level, 1)

        XCTAssertTrue(adapter.moveDungeonSession(&session, command: UInt16(UInt8(ascii: "K")), result: &result))
        XCTAssertTrue(result.level_changed)
        XCTAssertTrue(result.needs_redraw)
        XCTAssertFalse(result.exited)
        XCTAssertEqual(session.descriptor.dungeon_level, 0)
    }

    func testLoadLocationSessionRejectsInvalidRequestWithoutProducingSession() {
        let adapter = ShellResourceAdapter()
        var request = makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_TOWN), index: 2, x: 1, y: 32, heading: 2)
        request.resource_id = 403

        let result = adapter.loadLocationSession(resourceRootPath: resourceRootPath, request: request)

        guard case .failure(let status) = result else {
            return XCTFail("Expected invalid request failure")
        }
        XCTAssertEqual(status, "Location records 403 invalid")
    }

    private func makeLocationRequest(
        kind: UInt8,
        index: UInt8,
        x: UInt8,
        y: UInt8,
        heading: UInt8
    ) -> u3_location_transition_result {
        var request = u3_location_transition_result()
        request.handled = 1
        request.requested = 1
        request.destination_kind = kind
        request.location_index = index
        request.resource_id = u3_location_resource_id_for_index(index)
        request.return_x = 46
        request.return_y = 19
        request.initial_x = x
        request.initial_y = y
        request.initial_heading = heading
        return request
    }

    private func countCommands(in frame: u3_render_frame, value: Int) -> Int {
        var frame = frame
        let commandCount = min(Int(frame.command_count), Int(U3_RENDER_MAX_COMMANDS))
        return withUnsafePointer(to: &frame.commands) { pointer in
            pointer.withMemoryRebound(to: u3_render_command.self, capacity: commandCount) { commands in
                (0..<commandCount).reduce(0) { count, index in
                    count + (Int(commands[index].value) == value ? 1 : 0)
                }
            }
        }
    }
}
