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

    func testApplyTownHealerServiceMutatesSelectedRoster() throws {
        let adapter = ShellResourceAdapter()
        var document: Data? = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        var updatedDocument = try XCTUnwrap(document)
        XCTAssertTrue(setRosterHitPoints(rosterID: 1, hitPoints: 50, in: &updatedDocument))
        XCTAssertTrue(setRosterGold(rosterID: 1, gold: 250, in: &updatedDocument))
        document = updatedDocument
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_TOWN), index: 2, x: 1, y: 32, heading: 2)
        )
        guard case .success(let town) = loadResult else {
            return XCTFail("Expected LCB Towne session")
        }

        let prompt = adapter.applyTownService(
            town,
            documentData: &document,
            command: UInt16(UInt8(ascii: "H")),
            selectedActiveSlot: 0
        )
        XCTAssertEqual(prompt.service.status, UInt8(U3_TOWN_SERVICE_STATUS_SELECTION_REQUIRED))
        XCTAssertEqual(prompt.message, "Healer: choose character")
        XCTAssertFalse(prompt.documentMutated)

        let healed = adapter.applyTownService(
            town,
            documentData: &document,
            command: UInt16(UInt8(ascii: "H")),
            selectedActiveSlot: 1
        )

        XCTAssertEqual(healed.service.status, UInt8(U3_TOWN_SERVICE_STATUS_HEALED))
        XCTAssertEqual(healed.service.hit_points_after, 100)
        XCTAssertEqual(healed.service.gold_after, 50)
        XCTAssertEqual(healed.message, "Healer healed roster 1 HP 100 gold 50 cost 200")
        XCTAssertTrue(healed.documentMutated)
        let healedDocument = try XCTUnwrap(document)
        XCTAssertEqual(rosterHitPoints(rosterID: 1, in: healedDocument), 100)
        XCTAssertEqual(rosterGold(rosterID: 1, in: healedDocument), 50)
    }

    func testApplyTownHealerServiceRejectsInsufficientGoldAndCancelWithoutMutation() throws {
        let adapter = ShellResourceAdapter()
        var document: Data? = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        var updatedDocument = try XCTUnwrap(document)
        XCTAssertTrue(setRosterHitPoints(rosterID: 1, hitPoints: 50, in: &updatedDocument))
        document = updatedDocument
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_TOWN), index: 2, x: 1, y: 32, heading: 2)
        )
        guard case .success(let town) = loadResult else {
            return XCTFail("Expected LCB Towne session")
        }

        let insufficient = adapter.applyTownService(
            town,
            documentData: &document,
            command: UInt16(UInt8(ascii: "H")),
            selectedActiveSlot: 1
        )
        XCTAssertEqual(insufficient.service.status, UInt8(U3_TOWN_SERVICE_STATUS_INSUFFICIENT_GOLD))
        XCTAssertEqual(insufficient.message, "Healer insufficient gold roster 1 gold 150 cost 200")
        XCTAssertFalse(insufficient.documentMutated)
        let insufficientDocument = try XCTUnwrap(document)
        XCTAssertEqual(rosterHitPoints(rosterID: 1, in: insufficientDocument), 50)
        XCTAssertEqual(rosterGold(rosterID: 1, in: insufficientDocument), 150)

        let cancelled = adapter.applyTownService(
            town,
            documentData: &document,
            command: UInt16(UInt8(ascii: "H")),
            selectedActiveSlot: UInt8(U3_PARTY_ACTIVE_SLOT_COUNT + 1)
        )
        XCTAssertEqual(cancelled.service.status, UInt8(U3_TOWN_SERVICE_STATUS_CANCELLED))
        XCTAssertFalse(cancelled.documentMutated)
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

    func testApplyDungeonPostTurnDecaysLightWithoutEncounter() throws {
        let adapter = ShellResourceAdapter()
        let document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(var dungeon) = loadResult else {
            return XCTFail("Expected dungeon session")
        }
        dungeon.descriptor.light = UInt8(U3_DUNGEON_LIGHT_FULL)

        let result = adapter.applyDungeonPostTurn(
            &dungeon,
            documentData: document,
            encounterRoll: 0,
            monsterRoll: 0
        )

        XCTAssertEqual(result.light_before, UInt8(U3_DUNGEON_LIGHT_FULL))
        XCTAssertEqual(result.light_after, UInt8(U3_DUNGEON_LIGHT_FULL - 1))
        XCTAssertEqual(result.light_decremented, 1)
        XCTAssertEqual(result.encounter_requested, 0)
        XCTAssertEqual(dungeon.descriptor.light, UInt8(U3_DUNGEON_LIGHT_FULL - 1))
    }

    func testApplyDungeonPostTurnReportsEncounterMetadata() throws {
        let adapter = ShellResourceAdapter()
        let document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(var dungeon) = loadResult else {
            return XCTFail("Expected dungeon session")
        }
        dungeon = dungeonSessionOnFirstFloorTile(dungeon)

        let result = adapter.applyDungeonPostTurn(
            &dungeon,
            documentData: document,
            encounterRoll: 128,
            monsterRoll: 2
        )

        XCTAssertEqual(result.encounter_requested, 1)
        XCTAssertEqual(result.monster_table_value, 0x1A)
        XCTAssertEqual(result.monster_type, 0x34)
        XCTAssertEqual(result.marker_tile, UInt8(U3_DUNGEON_ENCOUNTER_MARKER_TILE))
        XCTAssertEqual(result.combat_screen_resource_id, UInt16(U3_DUNGEON_COMBAT_SCREEN_RESOURCE_ID))
    }

    func testLoadCombatSessionCopiesScreenAndSourceDungeonSnapshot() throws {
        let adapter = ShellResourceAdapter()
        let document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(var dungeon) = loadResult else {
            return XCTFail("Expected dungeon session")
        }
        dungeon = dungeonSessionOnFirstFloorTile(dungeon)
        dungeon.descriptor.light = 9

        let encounter = adapter.applyDungeonPostTurn(
            &dungeon,
            documentData: document,
            encounterRoll: 128,
            monsterRoll: 2
        )
        let result = adapter.loadCombatSession(
            resourceRootPath: resourceRootPath,
            encounter: encounter,
            sourceSession: dungeon,
            documentData: document
        )

        guard case .success(let combat) = result else {
            return XCTFail("Expected combat session")
        }
        XCTAssertEqual(combat.screenResourceID, UInt16(U3_DUNGEON_COMBAT_SCREEN_RESOURCE_ID))
        XCTAssertGreaterThanOrEqual(combat.screenData.count, renderTileCount)
        XCTAssertEqual(combat.monsterTableValue, 0x1A)
        XCTAssertEqual(combat.monsterType, 0x34)
        XCTAssertEqual(combat.markerTile, UInt8(U3_DUNGEON_ENCOUNTER_MARKER_TILE))
        XCTAssertEqual(combat.partySize, 4)
        XCTAssertEqual(combat.activeRosterIDs.0, 1)
        XCTAssertEqual(combat.activeRosterIDs.1, 2)
        XCTAssertEqual(combat.activeRosterIDs.2, 3)
        XCTAssertEqual(combat.activeRosterIDs.3, 4)
        XCTAssertEqual(combat.screenInit.status, UInt8(U3_COMBAT_SCREEN_STATUS_OK))
        XCTAssertEqual(combat.renderResult.terrain_commands, UInt8(U3_RENDER_TILE_COUNT))
        XCTAssertEqual(combat.renderResult.monster_commands, 1)
        XCTAssertEqual(combat.renderResult.party_commands, 4)
        XCTAssertEqual(combat.frame.command_count, UInt16(U3_RENDER_TILE_COUNT + 2 + 1 + 4))
        XCTAssertEqual(renderCommand(in: combat.frame, index: 123).value, UInt16(U3_COMBAT_RENDER_MONSTER_BASE))
        XCTAssertEqual(renderCommand(in: combat.frame, index: 124).value, UInt16(U3_COMBAT_RENDER_PARTY_BASE))
        XCTAssertEqual(combat.sourceLocationSession.descriptor.resource_id, dungeon.descriptor.resource_id)
        XCTAssertEqual(combat.sourceLocationSession.descriptor.x, dungeon.descriptor.x)
        XCTAssertEqual(combat.sourceLocationSession.descriptor.y, dungeon.descriptor.y)
        XCTAssertEqual(combat.sourceLocationSession.descriptor.heading, dungeon.descriptor.heading)
        XCTAssertEqual(combat.sourceLocationSession.descriptor.light, dungeon.descriptor.light)
        XCTAssertEqual(combat.sourceLocationSession.mapData, dungeon.mapData)
        XCTAssertEqual(combat.status, "Combat OK CONS 402 monster 52 marker 64 party 4 active 1/2/3/4 terrain 121 monsters 1 characters 4 source dungeon MAPS 412")
    }

    func testLoadCombatSessionHidesIncapacitatedPartyMembers() throws {
        let adapter = ShellResourceAdapter()
        var document = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        let deadOffset = rosterPayloadOffset(in: document, rosterID: 1)
        let ashOffset = rosterPayloadOffset(in: document, rosterID: 2)
        XCTAssertGreaterThan(deadOffset, 0)
        XCTAssertGreaterThan(ashOffset, 0)
        document[deadOffset + Int(U3_PARTY_ROSTER_STATUS_OFFSET)] = UInt8(ascii: "D")
        document[ashOffset + Int(U3_PARTY_ROSTER_STATUS_OFFSET)] = UInt8(ascii: "A")

        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(var dungeon) = loadResult else {
            return XCTFail("Expected dungeon session")
        }
        dungeon = dungeonSessionOnFirstFloorTile(dungeon)

        let encounter = adapter.applyDungeonPostTurn(
            &dungeon,
            documentData: document,
            encounterRoll: 128,
            monsterRoll: 2
        )
        let result = adapter.loadCombatSession(
            resourceRootPath: resourceRootPath,
            encounter: encounter,
            sourceSession: dungeon,
            documentData: document
        )

        guard case .success(let combat) = result else {
            return XCTFail("Expected combat session")
        }
        XCTAssertEqual(combat.renderResult.party_commands, 2)
        XCTAssertEqual(combat.frame.command_count, UInt16(U3_RENDER_TILE_COUNT + 2 + 1 + 2))
        XCTAssertEqual(combat.combatState.character_x.0, 0xFF)
        XCTAssertEqual(combat.combatState.character_y.0, 0xFF)
        XCTAssertEqual(combat.combatState.character_shape.0, 0)
        XCTAssertEqual(combat.combatState.character_x.1, 0xFF)
        XCTAssertEqual(combat.combatState.character_y.1, 0xFF)
        XCTAssertEqual(combat.combatState.character_shape.1, 0)
    }

    func testLoadCombatSessionRejectsMissingRequestAndResources() throws {
        let adapter = ShellResourceAdapter()
        let loadResult = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(let dungeon) = loadResult else {
            return XCTFail("Expected dungeon session")
        }

        XCTAssertEqual(
            failureMessage(adapter.loadCombatSession(
                resourceRootPath: resourceRootPath,
                encounter: u3_dungeon_post_turn_result(),
                sourceSession: dungeon,
                documentData: nil
            )),
            "Combat request missing"
        )

        var encounter = u3_dungeon_post_turn_result()
        encounter.encounter_requested = 1
        encounter.combat_screen_resource_id = UInt16(U3_DUNGEON_COMBAT_SCREEN_RESOURCE_ID)
        XCTAssertEqual(
            failureMessage(adapter.loadCombatSession(
                resourceRootPath: nil,
                encounter: encounter,
                sourceSession: dungeon,
                documentData: nil
            )),
            "Combat resource root missing"
        )

        encounter.combat_screen_resource_id = 999
        XCTAssertEqual(
            failureMessage(adapter.loadCombatSession(
                resourceRootPath: resourceRootPath,
                encounter: encounter,
                sourceSession: dungeon,
                documentData: nil
            )),
            "Combat CONS 999 missing"
        )

        encounter.combat_screen_resource_id = 410
        XCTAssertEqual(
            failureMessage(adapter.loadCombatSession(
                resourceRootPath: resourceRootPath,
                encounter: encounter,
                sourceSession: dungeon,
                documentData: nil
            )),
            "Combat CONS 410 invalid"
        )
    }

    func testApplyDungeonSpecialWindClearsLightAndRefreshesTransientFrame() throws {
        let adapter = ShellResourceAdapter()
        var document: Data? = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        var dungeon = try loadDungeonSession(adapter: adapter)
        dungeon.descriptor.light = 7
        setDungeonTile(UInt8(U3_DUNGEON_TILE_WIND), in: &dungeon)

        let result = adapter.applyDungeonSpecialEffect(
            &dungeon,
            documentData: &document,
            disarmRoll: 0,
            gremlinRoll: 0,
            trapDamageRoll: 0
        )

        XCTAssertEqual(result.effect.status, UInt8(U3_DUNGEON_SPECIAL_STATUS_WIND))
        XCTAssertEqual(dungeon.descriptor.light, 0)
        XCTAssertTrue(result.sessionMutated)
        XCTAssertFalse(result.documentMutated)
        XCTAssertEqual(dungeon.frame.command_count, 2)
    }

    func testApplyDungeonSpecialTrapClearsTransientTileAndMutatesRosterOnFailure() throws {
        let adapter = ShellResourceAdapter()
        var document: Data? = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        var dungeon = try loadDungeonSession(adapter: adapter)
        setDungeonTile(UInt8(U3_DUNGEON_TILE_TRAP), in: &dungeon)

        let result = adapter.applyDungeonSpecialEffect(
            &dungeon,
            documentData: &document,
            disarmRoll: 0,
            gremlinRoll: 0,
            trapDamageRoll: 0x77
        )

        XCTAssertEqual(result.effect.status, UInt8(U3_DUNGEON_SPECIAL_STATUS_TRAP_DAMAGE))
        XCTAssertEqual(result.effect.clear_current_tile, 1)
        XCTAssertTrue(result.sessionMutated)
        XCTAssertTrue(result.documentMutated)
        XCTAssertEqual(currentDungeonTile(in: dungeon), 0)
        let updatedDocument = try XCTUnwrap(document)
        XCTAssertLessThan(rosterHitPoints(rosterID: 1, in: updatedDocument), 100)
    }

    func testApplyDungeonSpecialGremlinsMutatesFoodWithoutUnderflow() throws {
        let adapter = ShellResourceAdapter()
        var currentDocument = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setRosterFood(rosterID: 1, hundreds: 0, remainder: 50, in: &currentDocument))
        var document: Data? = currentDocument
        var dungeon = try loadDungeonSession(adapter: adapter)
        setDungeonTile(UInt8(U3_DUNGEON_TILE_GREMLINS), in: &dungeon)

        let result = adapter.applyDungeonSpecialEffect(
            &dungeon,
            documentData: &document,
            disarmRoll: 0,
            gremlinRoll: 0,
            trapDamageRoll: 0
        )

        XCTAssertEqual(result.effect.status, UInt8(U3_DUNGEON_SPECIAL_STATUS_GREMLINS))
        XCTAssertEqual(result.effect.roster_id, 1)
        XCTAssertEqual(result.effect.food_before, 50)
        XCTAssertEqual(result.effect.food_after, 50)
        XCTAssertTrue(result.documentMutated)
        let updatedDocument = try XCTUnwrap(document)
        XCTAssertEqual(rosterFood(rosterID: 1, in: updatedDocument), 50)
        XCTAssertEqual(currentDungeonTile(in: dungeon), 0)
    }

    func testApplyDungeonSpecialWritingUsesBundledDungeonTalkEntry() throws {
        let adapter = ShellResourceAdapter()
        var document: Data? = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        var dungeon = try loadDungeonSession(adapter: adapter)
        setDungeonTile(UInt8(U3_DUNGEON_TILE_WRITING), in: &dungeon)

        let result = adapter.applyDungeonSpecialEffect(
            &dungeon,
            documentData: &document,
            disarmRoll: 0,
            gremlinRoll: 0,
            trapDamageRoll: 0
        )

        XCTAssertEqual(result.effect.status, UInt8(U3_DUNGEON_SPECIAL_STATUS_WRITING))
        XCTAssertEqual(result.effect.message_id, 164)
        XCTAssertTrue(result.message?.hasPrefix("Writing: ") == true)
        XCTAssertFalse(result.documentMutated)
        XCTAssertFalse(result.sessionMutated)
    }

    func testApplyDungeonInteractionFountainMutatesSelectedRoster() throws {
        let adapter = ShellResourceAdapter()
        var currentDocument = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        XCTAssertTrue(setRosterHitPoints(rosterID: 2, hitPoints: 25, in: &currentDocument))
        var document: Data? = currentDocument
        var dungeon = try loadDungeonSession(adapter: adapter)
        dungeon.descriptor.x = 1
        setDungeonTile(UInt8(U3_DUNGEON_TILE_FOUNTAIN), in: &dungeon)

        let prompt = adapter.applyDungeonInteraction(
            &dungeon,
            documentData: &document,
            command: 0,
            selectedActiveSlot: 0,
            chestTrapRoll: 0,
            chestGoldRoll: 0
        )
        XCTAssertEqual(prompt.interaction.status, UInt8(U3_DUNGEON_INTERACTION_STATUS_SELECTION_REQUIRED))
        XCTAssertFalse(prompt.documentMutated)

        let result = adapter.applyDungeonInteraction(
            &dungeon,
            documentData: &document,
            command: 0,
            selectedActiveSlot: 2,
            chestTrapRoll: 0,
            chestGoldRoll: 0
        )

        XCTAssertEqual(result.interaction.status, UInt8(U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_HEAL))
        XCTAssertTrue(result.documentMutated)
        XCTAssertFalse(result.sessionMutated)
        let updatedDocument = try XCTUnwrap(document)
        XCTAssertEqual(rosterHitPoints(rosterID: 2, in: updatedDocument), 100)
    }

    func testApplyDungeonInteractionMarkAndChestMutations() throws {
        let adapter = ShellResourceAdapter()
        var document: Data? = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        var dungeon = try loadDungeonSession(adapter: adapter)
        dungeon.descriptor.x = 2
        setDungeonTile(UInt8(U3_DUNGEON_TILE_MARK), in: &dungeon)

        let mark = adapter.applyDungeonInteraction(
            &dungeon,
            documentData: &document,
            command: 0,
            selectedActiveSlot: 1,
            chestTrapRoll: 0,
            chestGoldRoll: 0
        )
        XCTAssertEqual(mark.interaction.status, UInt8(U3_DUNGEON_INTERACTION_STATUS_MARK))
        XCTAssertEqual(mark.interaction.mark_after & 64, 64)
        XCTAssertTrue(mark.documentMutated)

        setDungeonTile(UInt8(U3_DUNGEON_TILE_CHEST), in: &dungeon)
        let chest = adapter.applyDungeonInteraction(
            &dungeon,
            documentData: &document,
            command: UInt16(UInt8(ascii: "G")),
            selectedActiveSlot: 1,
            chestTrapRoll: 0,
            chestGoldRoll: 20
        )

        XCTAssertEqual(chest.interaction.status, UInt8(U3_DUNGEON_INTERACTION_STATUS_CHEST_OPENED))
        XCTAssertEqual(chest.interaction.gold_added, 50)
        XCTAssertTrue(chest.documentMutated)
        XCTAssertTrue(chest.sessionMutated)
        XCTAssertEqual(currentDungeonTile(in: dungeon), 0)

        setDungeonTile(UInt8(U3_DUNGEON_TILE_CHEST), in: &dungeon)
        let trappedChest = adapter.applyDungeonInteraction(
            &dungeon,
            documentData: &document,
            command: UInt16(UInt8(ascii: "G")),
            selectedActiveSlot: 1,
            chestTrapRoll: 200,
            chestGoldRoll: UInt16((5 << 8) | 20)
        )

        XCTAssertEqual(trappedChest.interaction.status, UInt8(U3_DUNGEON_INTERACTION_STATUS_CHEST_OPENED))
        XCTAssertEqual(trappedChest.interaction.chest_trap_kind, UInt8(U3_DUNGEON_CHEST_TRAP_DAMAGE))
        XCTAssertEqual(trappedChest.interaction.chest_trap_damage, 8)
        XCTAssertEqual(trappedChest.interaction.hit_points_after, 42)
        XCTAssertEqual(trappedChest.interaction.weapon_reward, 5)
        XCTAssertEqual(trappedChest.interaction.weapon_after, 1)
        XCTAssertTrue(trappedChest.documentMutated)
        let trappedDocument = try XCTUnwrap(document)
        XCTAssertEqual(rosterHitPoints(rosterID: 1, in: trappedDocument), 42)
        XCTAssertEqual(rosterByte(rosterID: 1, offset: 48 + 5, in: trappedDocument), 1)

        setDungeonTile(UInt8(U3_DUNGEON_TILE_CHEST), in: &dungeon)
        let armourChest = adapter.applyDungeonInteraction(
            &dungeon,
            documentData: &document,
            command: UInt16(UInt8(ascii: "G")),
            selectedActiveSlot: 1,
            chestTrapRoll: 0,
            chestGoldRoll: UInt16((131 << 8) | 20)
        )

        XCTAssertEqual(armourChest.interaction.armour_reward, 3)
        XCTAssertEqual(armourChest.interaction.armour_after, 1)
        let armourDocument = try XCTUnwrap(document)
        XCTAssertEqual(rosterByte(rosterID: 1, offset: 40 + 3, in: armourDocument), 1)
    }

    func testApplyDungeonInteractionTimeLordAndCancelDoNotMutate() throws {
        let adapter = ShellResourceAdapter()
        var document: Data? = try XCTUnwrap(adapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        var dungeon = try loadDungeonSession(adapter: adapter)
        setDungeonTile(UInt8(U3_DUNGEON_TILE_TIME_LORD), in: &dungeon)

        let timeLord = adapter.applyDungeonInteraction(
            &dungeon,
            documentData: &document,
            command: 0,
            selectedActiveSlot: 0,
            chestTrapRoll: 0,
            chestGoldRoll: 0
        )
        XCTAssertEqual(timeLord.interaction.status, UInt8(U3_DUNGEON_INTERACTION_STATUS_TIME_LORD))
        XCTAssertEqual(timeLord.message, "Time Lord message 151")
        XCTAssertFalse(timeLord.documentMutated)

        setDungeonTile(UInt8(U3_DUNGEON_TILE_FOUNTAIN), in: &dungeon)
        let cancelled = adapter.applyDungeonInteraction(
            &dungeon,
            documentData: &document,
            command: 0,
            selectedActiveSlot: UInt8(U3_PARTY_ACTIVE_SLOT_COUNT + 1),
            chestTrapRoll: 0,
            chestGoldRoll: 0
        )
        XCTAssertEqual(cancelled.interaction.status, UInt8(U3_DUNGEON_INTERACTION_STATUS_CANCELLED))
        XCTAssertFalse(cancelled.documentMutated)
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

    private func loadDungeonSession(adapter: ShellResourceAdapter) throws -> ShellLocationSession {
        let result = adapter.loadLocationSession(
            resourceRootPath: resourceRootPath,
            request: makeLocationRequest(kind: UInt8(U3_LOCATION_KIND_DUNGEON), index: 12, x: 1, y: 1, heading: 1)
        )
        guard case .success(let dungeon) = result else {
            throw XCTSkip("Expected dungeon session")
        }
        return dungeon
    }

    private func setDungeonTile(_ tile: UInt8, in session: inout ShellLocationSession) {
        let offset = (Int(session.descriptor.dungeon_level) * Int(U3_DUNGEON_WIDTH) * Int(U3_DUNGEON_HEIGHT)) +
            (Int(session.descriptor.y) * Int(U3_DUNGEON_WIDTH)) +
            Int(session.descriptor.x)
        session.mapData[offset] = tile
    }

    private func currentDungeonTile(in session: ShellLocationSession) -> UInt8 {
        let offset = (Int(session.descriptor.dungeon_level) * Int(U3_DUNGEON_WIDTH) * Int(U3_DUNGEON_HEIGHT)) +
            (Int(session.descriptor.y) * Int(U3_DUNGEON_WIDTH)) +
            Int(session.descriptor.x)
        return session.mapData[offset]
    }

    private func rosterHitPoints(rosterID: UInt8, in documentData: Data) -> UInt16 {
        guard let record = rosterRecord(rosterID: rosterID, in: documentData) else {
            return 0
        }
        return (UInt16(record[26]) << 8) | UInt16(record[27])
    }

    private func rosterGold(rosterID: UInt8, in documentData: Data) -> UInt16 {
        guard let record = rosterRecord(rosterID: rosterID, in: documentData) else {
            return 0
        }
        return (UInt16(record[35]) << 8) | UInt16(record[36])
    }

    private func rosterFood(rosterID: UInt8, in documentData: Data) -> UInt16 {
        guard let record = rosterRecord(rosterID: rosterID, in: documentData) else {
            return 0
        }
        return UInt16(record[32]) * 100 + UInt16(record[33])
    }

    private func rosterByte(rosterID: UInt8, offset: Int, in documentData: Data) -> UInt8 {
        guard let record = rosterRecord(rosterID: rosterID, in: documentData),
              offset >= 0,
              offset < record.count else {
            return 0
        }
        return record[offset]
    }

    private func setRosterFood(rosterID: UInt8, hundreds: UInt8, remainder: UInt8, in documentData: inout Data) -> Bool {
        let offset = rosterPayloadOffset(in: documentData, rosterID: rosterID)
        guard offset > 0, offset + Int(U3_PARTY_ROSTER_RECORD_LENGTH) <= documentData.count else {
            return false
        }
        documentData[offset + 32] = hundreds
        documentData[offset + 33] = remainder
        return true
    }

    private func setRosterHitPoints(rosterID: UInt8, hitPoints: UInt16, in documentData: inout Data) -> Bool {
        let offset = rosterPayloadOffset(in: documentData, rosterID: rosterID)
        guard offset > 0, offset + Int(U3_PARTY_ROSTER_RECORD_LENGTH) <= documentData.count else {
            return false
        }
        documentData[offset + 26] = UInt8(hitPoints >> 8)
        documentData[offset + 27] = UInt8(hitPoints & 0xff)
        return true
    }

    private func setRosterGold(rosterID: UInt8, gold: UInt16, in documentData: inout Data) -> Bool {
        let offset = rosterPayloadOffset(in: documentData, rosterID: rosterID)
        guard offset > 0, offset + Int(U3_PARTY_ROSTER_RECORD_LENGTH) <= documentData.count else {
            return false
        }
        documentData[offset + 35] = UInt8(gold >> 8)
        documentData[offset + 36] = UInt8(gold & 0xff)
        return true
    }

    private func rosterRecord(rosterID: UInt8, in documentData: Data) -> [UInt8]? {
        let offset = rosterPayloadOffset(in: documentData, rosterID: rosterID)
        guard offset > 0, offset + Int(U3_PARTY_ROSTER_RECORD_LENGTH) <= documentData.count else {
            return nil
        }
        return Array(documentData[offset..<(offset + Int(U3_PARTY_ROSTER_RECORD_LENGTH))])
    }

    private func rosterPayloadOffset(in documentData: Data, rosterID: UInt8) -> Int {
        documentData.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return 0
            }
            var document = u3_save_document()
            var record = u3_save_record()
            guard u3_save_open(baseAddress, documentData.count, &document) != 0,
                  u3_save_find_record(&document, fourCharacterCode("ROST"), Int16(U3_SAVE_ID_ROSTER), &record) != 0,
                  record.length == U3_SAVE_ROSTER_LENGTH,
                  let recordData = record.data,
                  rosterID > 0,
                  rosterID <= UInt8(U3_PARTY_ROSTER_SLOT_COUNT) else {
                return 0
            }
            return baseAddress.distance(to: recordData) + ((Int(rosterID) - 1) * Int(U3_PARTY_ROSTER_RECORD_LENGTH))
        }
    }

    private func fourCharacterCode(_ value: String) -> UInt32 {
        var result: UInt32 = 0

        for byte in value.utf8.prefix(4) {
            result = (result << 8) | UInt32(byte)
        }

        return result
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

    private func renderCommand(in frame: u3_render_frame, index: Int) -> u3_render_command {
        var frame = frame
        return withUnsafePointer(to: &frame.commands) { pointer in
            pointer.withMemoryRebound(to: u3_render_command.self, capacity: Int(U3_RENDER_MAX_COMMANDS)) { commands in
                commands[index]
            }
        }
    }

    private func failureMessage(_ result: ShellCombatSessionLoadResult) -> String? {
        if case .failure(let message) = result {
            return message
        }
        return nil
    }

    private func dungeonSessionOnFirstFloorTile(_ session: ShellLocationSession) -> ShellLocationSession {
        var session = session
        for y in UInt8(0)..<UInt8(U3_DUNGEON_HEIGHT) {
            for x in UInt8(0)..<UInt8(U3_DUNGEON_WIDTH) {
                let offset = (Int(session.descriptor.dungeon_level) * Int(U3_DUNGEON_WIDTH) * Int(U3_DUNGEON_HEIGHT)) +
                    (Int(y) * Int(U3_DUNGEON_WIDTH)) +
                    Int(x)
                if session.mapData[offset] == 0 {
                    session.descriptor.x = x
                    session.descriptor.y = y
                    return session
                }
            }
        }
        return session
    }
}
