import Foundation
import Ultima3Core

struct ShellRenderSmokeResult {
    let frame: u3_render_frame
    let status: String
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

        return documentData.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return "New Game Empty smoke document"
            }

            var document = u3_save_document()
            guard u3_save_open(baseAddress, documentData.count, &document) != 0 else {
                return "New Game Invalid smoke document"
            }

            var state = u3_save_domain_state()
            guard u3_save_load_domain_state(&document, &state) != 0 else {
                return "New Game Failed load domain state"
            }

            let miscLengths = [
                state.misc_length.0,
                state.misc_length.1,
                state.misc_length.2,
                state.misc_length.3,
                state.misc_length.4,
                state.misc_length.5
            ]
            let miscStatus = miscLengths.map(String.init).joined(separator: "/")
            return "New Game OK party \(state.party_length) roster \(state.roster_length) map \(state.current_sosaria_map_length) creatures \(state.current_sosaria_creatures_length) misc \(miscStatus)"
        }
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

    private func fourCharacterCode(_ value: String) -> UInt32 {
        var result: UInt32 = 0

        for byte in value.utf8.prefix(4) {
            result = (result << 8) | UInt32(byte)
        }

        return result
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
