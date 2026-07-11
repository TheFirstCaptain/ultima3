import Foundation
import Ultima3Core

struct ShellRenderSmokeResult {
    let frame: u3_render_frame
    let status: String
}

struct ShellOverworldSmokeResult {
    let map: Data?
    let frame: u3_render_frame
    let status: String
}

struct ShellLocationSession {
    var descriptor: u3_location_session
    let mapData: Data
    let monsterData: Data?
    let talkData: Data
    var frame: u3_render_frame

    var status: String {
        if descriptor.destination_kind == U3_LOCATION_KIND_DUNGEON {
            return "Dungeon OK MAPS \(descriptor.resource_id) level \(descriptor.dungeon_level) pos \(descriptor.x),\(descriptor.y) heading \(descriptor.heading)"
        }
        return "Location OK MAPS \(descriptor.resource_id) kind \(descriptor.destination_kind) pos \(descriptor.x),\(descriptor.y)"
    }
}

enum ShellLocationSessionLoadResult {
    case success(ShellLocationSession)
    case failure(String)
}

final class ShellResourceAdapter {
    func validateMainResources(resourceRootPath: String?) -> String {
        guard let resourceRootPath else {
            return "Resource Missing root"
        }

        let resourceURL = mainResourcesURL(resourceRootPath: resourceRootPath)

        guard FileManager.default.fileExists(atPath: resourceURL.path) else {
            return "Resource Missing MainResources.rsrc"
        }

        do {
            let data = try Data(contentsOf: resourceURL)
            return data.withUnsafeBytes { rawBuffer in
                guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                    return "Resource Empty MainResources.rsrc"
                }

                var resourceFile = u3_resource_file()
                guard u3_resource_open(baseAddress, data.count, &resourceFile) != 0 else {
                    return "Resource Invalid MainResources.rsrc"
                }

                guard resourceFile.type_count == 45 else {
                    return "Resource Type Count \(resourceFile.type_count)"
                }

                var partyRecord = u3_resource_record()
                guard u3_resource_find(&resourceFile, fourCharacterCode("PRTY"), 500, &partyRecord) != 0,
                      partyRecord.length == 64 else {
                    return "Resource Missing PRTY 500"
                }

                var mapRecord = u3_resource_record()
                guard u3_resource_find(&resourceFile, fourCharacterCode("MAPS"), 420, &mapRecord) != 0,
                      mapRecord.length == 4101 else {
                    return "Resource Missing MAPS 420"
                }

                return "Resource OK MainResources.rsrc"
            }
        } catch {
            return "Resource Failed MainResources.rsrc"
        }
    }

    func buildNativeNewGameSmokeDocument(resourceRootPath: String?) -> Data? {
        guard let resourceRootPath else {
            return nil
        }

        let resourceURL = mainResourcesURL(resourceRootPath: resourceRootPath)
        guard FileManager.default.fileExists(atPath: resourceURL.path),
              let resourceData = try? Data(contentsOf: resourceURL) else {
            return nil
        }

        return resourceData.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return nil
            }

            var resourceFile = u3_resource_file()
            guard u3_resource_open(baseAddress, resourceData.count, &resourceFile) != 0 else {
                return nil
            }

            var templates = u3_save_templates()
            guard populateSaveTemplates(&templates, resourceFile: &resourceFile) else {
                return nil
            }

            let capacity = u3_save_new_game_fixture_size()
            var output = Data(count: capacity)
            let success = output.withUnsafeMutableBytes { outputBuffer in
                guard let outputBaseAddress = outputBuffer.bindMemory(to: UInt8.self).baseAddress else {
                    return false
                }

                var written = 0
                return u3_save_build_new_game_fixture(&templates, outputBaseAddress, capacity, &written) != 0 && written == capacity
            }

            return success ? output : nil
        }
    }

    func loadNewGameSmokeState(resourceRootPath: String?) -> String {
        guard let documentData = buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath) else {
            return "New Game Failed build smoke document"
        }

        return describeSaveDomain(documentData, prefix: "New Game")
    }

    func describeSaveDomain(_ documentData: Data, prefix: String) -> String {
        switch loadSaveDomain(documentData) {
        case .success(let state):
            let miscLengths = [
                state.misc_length.0,
                state.misc_length.1,
                state.misc_length.2,
                state.misc_length.3,
                state.misc_length.4,
                state.misc_length.5
            ]
            let miscStatus = miscLengths.map(String.init).joined(separator: "/")
            return "\(prefix) OK party \(state.party_length) roster \(state.roster_length) map \(state.current_sosaria_map_length) creatures \(state.current_sosaria_creatures_length) misc \(miscStatus)"
        case .failure(let status):
            return "\(prefix) \(status)"
        }
    }

    func isValidSaveDomain(_ documentData: Data) -> Bool {
        if case .success = loadSaveDomain(documentData) {
            return true
        }

        return false
    }

    func inspectPartyRoster(resourceRootPath: String?) -> String {
        guard let documentData = buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath) else {
            return "Party Failed build smoke document"
        }

        return inspectPartyRoster(documentData: documentData)
    }

    func inspectPartyRoster(documentData: Data) -> String {
        return documentData.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return "Party Empty smoke document"
            }

            var document = u3_save_document()
            guard u3_save_open(baseAddress, documentData.count, &document) != 0 else {
                return "Party Invalid smoke document"
            }

            var state = u3_save_domain_state()
            guard u3_save_load_domain_state(&document, &state) != 0 else {
                return "Party Failed load domain state"
            }

            var summary = u3_party_summary()
            guard u3_party_load_summary(&state, &summary) != 0 else {
                return "Party Failed decode roster"
            }

            return formatPartyRosterSummary(&summary)
        }
    }

    func currentSosariaSaveAllowed(documentData: Data) -> Bool {
        switch loadSaveDomain(documentData) {
        case .success(let state):
            guard let party = state.party,
                  state.party_length == U3_SAVE_PARTY_LENGTH else {
                return false
            }

            return party[2] == 0
        case .failure:
            return false
        }
    }

    func formatPartyRosterSummary(_ summary: inout u3_party_summary) -> String {
        let activeIDs = [
            summary.active_roster_ids.0,
            summary.active_roster_ids.1,
            summary.active_roster_ids.2,
            summary.active_roster_ids.3
        ]
        let activeStatus = activeIDs.map(String.init).joined(separator: "/")
        let leaderStatus: String
        if summary.party_size > 0,
           let leader = rosterEntry(rosterID: activeIDs[0], summary: &summary) {
            let leaderName = stringFromPartyName(leader.name)
            leaderStatus = "lead \(leaderName) \(characterCode(leader.status)) \(characterCode(leader.race))/\(characterCode(leader.character_class))/\(characterCode(leader.sex)) HP \(leader.hit_points)/\(leader.max_hit_points) L\(leader.level) food \(leader.food) gold \(leader.gold)"
        } else {
            leaderStatus = "lead none"
        }

        return "Party OK size \(summary.party_size) active \(activeStatus) occupied \(summary.occupied_roster_count) \(leaderStatus)"
    }

    func buildResourceBackedRenderSmokeFrame(resourceRootPath: String?) -> ShellRenderSmokeResult {
        let fallbackFrame = u3_render_make_synthetic_tile_frame()
        guard let resourceRootPath else {
            return ShellRenderSmokeResult(frame: fallbackFrame, status: "Render Missing resource root")
        }

        let resourceURL = mainResourcesURL(resourceRootPath: resourceRootPath)
        guard FileManager.default.fileExists(atPath: resourceURL.path),
              let resourceData = try? Data(contentsOf: resourceURL) else {
            return ShellRenderSmokeResult(frame: fallbackFrame, status: "Render Missing MainResources.rsrc")
        }

        return resourceData.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return ShellRenderSmokeResult(frame: fallbackFrame, status: "Render Empty MainResources.rsrc")
            }

            var resourceFile = u3_resource_file()
            guard u3_resource_open(baseAddress, resourceData.count, &resourceFile) != 0 else {
                return ShellRenderSmokeResult(frame: fallbackFrame, status: "Render Invalid MainResources.rsrc")
            }

            guard let combatScreen = findResource(resourceFile: &resourceFile, type: fourCharacterCode("CONS"), id: 400),
                  combatScreen.length >= U3_RENDER_TILE_COUNT else {
                return ShellRenderSmokeResult(frame: fallbackFrame, status: "Render Missing CONS 400")
            }

            let frame = u3_render_make_tile_grid_frame(combatScreen.data, UInt16(U3_RENDER_TILE_COUNT))
            return ShellRenderSmokeResult(frame: frame, status: "Render OK CONS 400 tiles \(U3_RENDER_TILE_COUNT)")
        }
    }

    func buildOverworldSmoke(resourceRootPath: String?, state: inout u3_overworld_state) -> ShellOverworldSmokeResult {
        let fallbackFrame = u3_render_make_synthetic_tile_frame()
        guard let documentData = buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath) else {
            return ShellOverworldSmokeResult(map: nil, frame: fallbackFrame, status: "Overworld Missing new game map")
        }

        return buildOverworldSmoke(documentData: documentData, state: &state)
    }

    func buildOverworldSmoke(documentData: Data, state: inout u3_overworld_state) -> ShellOverworldSmokeResult {
        let fallbackFrame = u3_render_make_synthetic_tile_frame()

        return documentData.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return ShellOverworldSmokeResult(map: nil, frame: fallbackFrame, status: "Overworld Empty new game map")
            }

            var document = u3_save_document()
            guard u3_save_open(baseAddress, documentData.count, &document) != 0 else {
                return ShellOverworldSmokeResult(map: nil, frame: fallbackFrame, status: "Overworld Invalid new game map")
            }

            var domainState = u3_save_domain_state()
            guard u3_save_load_domain_state(&document, &domainState) != 0,
                  let party = domainState.party,
                  let map = domainState.current_sosaria_map,
                  domainState.current_sosaria_map_length > 0 else {
                return ShellOverworldSmokeResult(map: nil, frame: fallbackFrame, status: "Overworld Failed load map")
            }

            let mapData = Data(bytes: map, count: Int(domainState.current_sosaria_map_length))
            guard Self.hasValidOverworldMapShape(mapData) else {
                return ShellOverworldSmokeResult(map: nil, frame: fallbackFrame, status: "Overworld Invalid MAPS 419 shape")
            }

            guard let mapSize = mapData.first,
                  u3_overworld_state_init_from_party(&state, party, domainState.party_length, mapSize) != 0 else {
                return ShellOverworldSmokeResult(map: nil, frame: fallbackFrame, status: "Overworld Failed load party position")
            }

            let frame = mapData.withUnsafeBytes { mapBuffer in
                guard let mapBaseAddress = mapBuffer.bindMemory(to: UInt8.self).baseAddress else {
                    return fallbackFrame
                }

                return u3_overworld_make_view_frame(mapBaseAddress, UInt32(mapData.count), &state)
            }
            return ShellOverworldSmokeResult(
                map: mapData,
                frame: frame,
                status: "Overworld OK MAPS 419 pos \(state.x),\(state.y)"
            )
        }
    }

    func handleOverworldLocationCommand(
        documentData: Data?,
        mapData: Data?,
        state: inout u3_overworld_state,
        command: UInt16,
        result: inout u3_location_transition_result
    ) -> Bool {
        guard let documentData, let mapData else {
            return false
        }

        return documentData.withUnsafeBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }

            var document = u3_save_document()
            guard u3_save_open(documentBaseAddress, documentData.count, &document) != 0 else {
                return false
            }

            var domainState = u3_save_domain_state()
            guard u3_save_load_domain_state(&document, &domainState) != 0,
                  let locationTable = domainState.misc.4,
                  domainState.misc_length.4 >= U3_LOCATION_TABLE_LENGTH else {
                return false
            }

            return mapData.withUnsafeBytes { mapBuffer in
                guard let mapBaseAddress = mapBuffer.bindMemory(to: UInt8.self).baseAddress else {
                    return false
                }

                return u3_location_handle_overworld_command(
                    &state,
                    mapBaseAddress,
                    UInt32(mapData.count),
                    locationTable,
                    domainState.misc_length.4,
                    command,
                    &result
                ) != 0
            }
        }
    }

    func loadLocationSession(
        resourceRootPath: String?,
        request: u3_location_transition_result
    ) -> ShellLocationSessionLoadResult {
        guard request.requested != 0 else {
            return .failure("Location request missing")
        }
        guard let resourceRootPath else {
            return .failure("Location resource root missing")
        }

        let resourceURL = mainResourcesURL(resourceRootPath: resourceRootPath)
        guard FileManager.default.fileExists(atPath: resourceURL.path),
              let resourceData = try? Data(contentsOf: resourceURL) else {
            return .failure("Location resources missing")
        }

        return resourceData.withUnsafeBytes { resourceBuffer in
            guard let resourceBaseAddress = resourceBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return .failure("Location resources empty")
            }

            var resourceFile = u3_resource_file()
            guard u3_resource_open(resourceBaseAddress, resourceData.count, &resourceFile) != 0 else {
                return .failure("Location resources invalid")
            }

            let resourceID = Int16(bitPattern: request.resource_id)
            guard let mapRecord = findResource(resourceFile: &resourceFile, type: fourCharacterCode("MAPS"), id: resourceID),
                  let mapPointer = mapRecord.data,
                  let talkRecord = findResource(resourceFile: &resourceFile, type: fourCharacterCode("TLKS"), id: resourceID),
                  let talkPointer = talkRecord.data else {
                return .failure("Location MAPS/TLKS \(request.resource_id) missing")
            }

            let monsterRecord: u3_resource_record?
            switch Int32(request.destination_kind) {
            case U3_LOCATION_KIND_TOWN, U3_LOCATION_KIND_CASTLE:
                monsterRecord = findResource(resourceFile: &resourceFile, type: fourCharacterCode("MONS"), id: resourceID)
                guard monsterRecord?.data != nil else {
                    return .failure("Location MONS \(request.resource_id) missing")
                }
            case U3_LOCATION_KIND_DUNGEON:
                monsterRecord = nil
            default:
                return .failure("Location kind \(request.destination_kind) unsupported")
            }

            var mutableRequest = request
            var descriptor = u3_location_session()
            guard u3_location_session_init(
                &mutableRequest,
                mapPointer,
                mapRecord.length,
                monsterRecord?.data,
                monsterRecord?.length ?? 0,
                talkPointer,
                talkRecord.length,
                &descriptor
            ) != 0 else {
                return .failure("Location records \(request.resource_id) invalid")
            }

            let mapData = Data(bytes: mapPointer, count: Int(mapRecord.length))
            let monsterData = monsterRecord.flatMap { record -> Data? in
                guard let pointer = record.data else {
                    return nil
                }
                return Data(bytes: pointer, count: Int(record.length))
            }
            let talkData = Data(bytes: talkPointer, count: Int(talkRecord.length))
            let frame = makeLocationFrame(descriptor: descriptor, mapData: mapData)
            return .success(ShellLocationSession(
                descriptor: descriptor,
                mapData: mapData,
                monsterData: monsterData,
                talkData: talkData,
                frame: frame
            ))
        }
    }

    func moveLocationSession(
        _ session: inout ShellLocationSession,
        command: UInt16,
        result: inout u3_location_move_result
    ) -> Bool {
        let moved = session.mapData.withUnsafeBytes { mapBuffer in
            guard let mapBaseAddress = mapBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }
            return u3_location_move(
                &session.descriptor,
                mapBaseAddress,
                UInt32(session.mapData.count),
                command,
                &result
            ) != 0
        }

        if moved && result.redraw != 0 {
            session.frame = makeLocationFrame(descriptor: session.descriptor, mapData: session.mapData)
        }
        return moved
    }

    func moveDungeonSession(
        _ session: inout ShellLocationSession,
        command: UInt16,
        result: inout u3_dungeon_result
    ) -> Bool {
        guard Self.validDungeonSessionForMovement(session) else {
            return false
        }

        var dungeonBytes = Array(session.mapData)
        let handled = dungeonBytes.withUnsafeMutableBufferPointer { dungeonBuffer in
            guard let dungeonBaseAddress = dungeonBuffer.baseAddress else {
                return false
            }
            var state = u3_dungeon_state(
                dungeon: dungeonBaseAddress,
                x: Int16(session.descriptor.x),
                y: Int16(session.descriptor.y),
                heading: Int16(session.descriptor.heading),
                level: Int16(session.descriptor.dungeon_level),
                exit_dungeon: false
            )

            switch Self.normalizedDungeonCommand(command) {
            case UInt16(U3_OVERWORLD_COMMAND_NORTH):
                result = u3_dungeon_forward(&state)
            case UInt16(U3_OVERWORLD_COMMAND_SOUTH):
                result = u3_dungeon_retreat(&state)
            case UInt16(U3_OVERWORLD_COMMAND_WEST):
                result = u3_dungeon_turn_left(&state)
            case UInt16(U3_OVERWORLD_COMMAND_EAST):
                result = u3_dungeon_turn_right(&state)
            case UInt16(UInt8(ascii: "D")):
                result = u3_dungeon_descend(&state)
            case UInt16(UInt8(ascii: "K")):
                result = u3_dungeon_climb(&state)
            default:
                return false
            }

            session.descriptor.x = UInt8(state.x)
            session.descriptor.y = UInt8(state.y)
            session.descriptor.heading = UInt8(state.heading)
            session.descriptor.dungeon_level = UInt8(state.level)
            return true
        }

        if handled && result.needs_redraw {
            session.frame = makeLocationFrame(descriptor: session.descriptor, mapData: session.mapData)
        }
        return handled
    }

    func talkLocationSession(
        _ session: ShellLocationSession,
        direction: UInt16,
        result: inout u3_location_talk_result
    ) -> Bool {
        guard session.descriptor.destination_kind == U3_LOCATION_KIND_TOWN,
              let monsterData = session.monsterData else {
            return false
        }

        return monsterData.withUnsafeBytes { monsterBuffer in
            guard let monsterBaseAddress = monsterBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }

            return session.talkData.withUnsafeBytes { talkBuffer in
                guard let talkBaseAddress = talkBuffer.bindMemory(to: UInt8.self).baseAddress else {
                    return false
                }

                var descriptor = session.descriptor
                return u3_location_talk(
                    &descriptor,
                    monsterBaseAddress,
                    UInt32(monsterData.count),
                    talkBaseAddress,
                    UInt32(session.talkData.count),
                    direction,
                    &result
                ) != 0
            }
        }
    }

    func locationTalkMessage(_ result: inout u3_location_talk_result) -> String? {
        guard result.status == U3_LOCATION_TALK_STATUS_MESSAGE,
              result.message_length > 0,
              result.message_length <= U3_LOCATION_TALK_MESSAGE_CAPACITY else {
            return nil
        }

        let bytes = withUnsafeBytes(of: &result.message) { buffer in
            Array(buffer.prefix(Int(result.message_length)))
        }
        return String(decoding: bytes, as: UTF8.self)
            .trimmingCharacters(in: .whitespacesAndNewlines)
            .replacingOccurrences(of: "\n", with: " / ")
    }

    func describeLocationTalk(_ result: inout u3_location_talk_result) -> String {
        switch Int32(result.status) {
        case U3_LOCATION_TALK_STATUS_MESSAGE:
            guard let message = locationTalkMessage(&result) else {
                return "Talk invalid message"
            }
            return "Talk: \(message)"
        case U3_LOCATION_TALK_STATUS_NO_NPC:
            return "Talk: no one at \(result.target_x),\(result.target_y)"
        case U3_LOCATION_TALK_STATUS_INVALID_INDEX:
            return "Talk: invalid entry \(result.talk_index)"
        case U3_LOCATION_TALK_STATUS_UNSUPPORTED:
            return "Talk: unsupported entry \(result.talk_index)"
        default:
            return "Talk: invalid direction"
        }
    }

    private func makeLocationFrame(descriptor: u3_location_session, mapData: Data) -> u3_render_frame {
        var descriptor = descriptor
        if Int32(descriptor.map_shape) == U3_LOCATION_MAP_SHAPE_DUNGEON {
            return mapData.withUnsafeBytes { mapBuffer in
                guard let mapBaseAddress = mapBuffer.bindMemory(to: UInt8.self).baseAddress else {
                    return u3_render_make_synthetic_tile_frame()
                }
                return u3_dungeon_make_view_frame(
                    mapBaseAddress,
                    UInt32(mapData.count),
                    Int16(descriptor.dungeon_level),
                    Int16(descriptor.x),
                    Int16(descriptor.y),
                    Int16(descriptor.heading)
                )
            }
        }

        guard Int32(descriptor.map_shape) == U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL else {
            return u3_render_make_synthetic_tile_frame()
        }

        return mapData.withUnsafeBytes { mapBuffer in
            guard let mapBaseAddress = mapBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return u3_render_make_synthetic_tile_frame()
            }
            return u3_location_make_view_frame(&descriptor, mapBaseAddress, UInt32(mapData.count))
        }
    }

    private static func normalizedDungeonCommand(_ command: UInt16) -> UInt16 {
        if command >= UInt16(UInt8(ascii: "a")) && command <= UInt16(UInt8(ascii: "z")) {
            return command - 32
        }
        return command
    }

    private static func validDungeonSessionForMovement(_ session: ShellLocationSession) -> Bool {
        session.descriptor.active != 0 &&
            session.descriptor.destination_kind == U3_LOCATION_KIND_DUNGEON &&
            session.descriptor.map_shape == U3_LOCATION_MAP_SHAPE_DUNGEON &&
            session.descriptor.map_size == U3_DUNGEON_WIDTH &&
            session.descriptor.map_length == U3_LOCATION_DUNGEON_MAP_LENGTH &&
            session.mapData.count == Int(U3_LOCATION_DUNGEON_MAP_LENGTH) &&
            session.descriptor.x < U3_DUNGEON_WIDTH &&
            session.descriptor.y < U3_DUNGEON_HEIGHT &&
            session.descriptor.heading < 4 &&
            session.descriptor.dungeon_level < U3_DUNGEON_LEVEL_COUNT
    }

    private static func hasValidOverworldMapShape(_ mapData: Data) -> Bool {
        guard let mapSize = mapData.first,
              mapSize > 0 else {
            return false
        }

        return mapData.count >= 1 + (Int(mapSize) * Int(mapSize))
    }

    private enum SaveDomainLoadResult {
        case success(u3_save_domain_state)
        case failure(String)
    }

    private func loadSaveDomain(_ documentData: Data) -> SaveDomainLoadResult {
        documentData.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return .failure("Empty smoke document")
            }

            var document = u3_save_document()
            guard u3_save_open(baseAddress, documentData.count, &document) != 0 else {
                return .failure("Invalid smoke document")
            }

            var state = u3_save_domain_state()
            guard u3_save_load_domain_state(&document, &state) != 0 else {
                return .failure("Failed load domain state")
            }

            return .success(state)
        }
    }

    private func fourCharacterCode(_ value: String) -> UInt32 {
        var result: UInt32 = 0

        for byte in value.utf8.prefix(4) {
            result = (result << 8) | UInt32(byte)
        }

        return result
    }

    private func stringFromPartyName(_ name: (Int8, Int8, Int8, Int8, Int8, Int8, Int8, Int8, Int8, Int8, Int8, Int8, Int8, Int8)) -> String {
        var name = name
        return withUnsafePointer(to: &name) { pointer in
            pointer.withMemoryRebound(to: CChar.self, capacity: 14) { characters in
                String(cString: characters)
            }
        }
    }

    private func characterCode(_ value: UInt8) -> String {
        guard value >= 32 && value <= 126,
              let scalar = UnicodeScalar(Int(value)) else {
            return "#\(value)"
        }

        return String(Character(scalar))
    }

    private func rosterEntry(rosterID: UInt8, summary: inout u3_party_summary) -> u3_party_roster_entry? {
        guard rosterID >= 1 && rosterID <= UInt8(U3_PARTY_ROSTER_SLOT_COUNT) else {
            return nil
        }

        return withUnsafePointer(to: &summary.roster) { pointer in
            pointer.withMemoryRebound(to: u3_party_roster_entry.self, capacity: Int(U3_PARTY_ROSTER_SLOT_COUNT)) { roster in
                roster[Int(rosterID) - 1]
            }
        }
    }

    private func mainResourcesURL(resourceRootPath: String) -> URL {
        URL(fileURLWithPath: resourceRootPath, isDirectory: true)
            .appendingPathComponent("English.lproj", isDirectory: true)
            .appendingPathComponent("MainResources.rsrc", isDirectory: false)
    }

    private func populateSaveTemplates(_ templates: inout u3_save_templates, resourceFile: inout u3_resource_file) -> Bool {
        guard let party = findResource(resourceFile: &resourceFile, type: fourCharacterCode("PRTY"), id: 500),
              party.length == U3_SAVE_PARTY_LENGTH,
              let roster = findResource(resourceFile: &resourceFile, type: fourCharacterCode("ROST"), id: 500),
              roster.length == U3_SAVE_ROSTER_LENGTH,
              let currentMap = findResource(resourceFile: &resourceFile, type: fourCharacterCode("MAPS"), id: 420),
              currentMap.length == U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH else {
            return false
        }

        templates.party = party.data
        templates.party_length = Int(party.length)
        templates.roster = roster.data
        templates.roster_length = Int(roster.length)
        templates.current_sosaria_map = currentMap.data
        templates.current_sosaria_map_length = Int(currentMap.length)

        for index in 0..<Int(U3_SAVE_MISC_TABLE_COUNT) {
            guard let misc = findResource(resourceFile: &resourceFile, type: fourCharacterCode("MISC"), id: Int16(400 + index)) else {
                return false
            }

            setMiscTemplate(&templates, index: index, data: misc.data, length: Int(misc.length))
        }

        return true
    }

    private func setMiscTemplate(_ templates: inout u3_save_templates, index: Int, data: UnsafePointer<UInt8>?, length: Int) {
        switch index {
        case 0:
            templates.misc.0 = data
            templates.misc_length.0 = length
        case 1:
            templates.misc.1 = data
            templates.misc_length.1 = length
        case 2:
            templates.misc.2 = data
            templates.misc_length.2 = length
        case 3:
            templates.misc.3 = data
            templates.misc_length.3 = length
        case 4:
            templates.misc.4 = data
            templates.misc_length.4 = length
        case 5:
            templates.misc.5 = data
            templates.misc_length.5 = length
        default:
            break
        }
    }

    private func findResource(resourceFile: inout u3_resource_file, type: UInt32, id: Int16) -> u3_resource_record? {
        var record = u3_resource_record()
        guard u3_resource_find(&resourceFile, type, id, &record) != 0 else {
            return nil
        }

        return record
    }
}
