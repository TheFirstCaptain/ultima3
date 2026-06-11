import Foundation
import Ultima3Core

struct ShellPartyRosterOption: Identifiable, Equatable {
    let id: UInt8
    let name: String
    let detail: String
}

struct ShellPartyAssemblyPresentation {
    let canAssemble: Bool
    let hasActiveParty: Bool
    let message: String
    let options: [ShellPartyRosterOption]
}

struct ShellPartyAssemblyResult {
    let assembled: Bool
    let message: String
    let document: Data?
    let activeRosterIDs: [UInt8]
}

final class ShellPartyAssemblyAdapter {
    func presentation(currentDocument: Data?) -> ShellPartyAssemblyPresentation {
        guard let currentDocument else {
            return ShellPartyAssemblyPresentation(
                canAssemble: false,
                hasActiveParty: false,
                message: "Party assembly requires current save",
                options: []
            )
        }

        return loadSummary(currentDocument) { summary in
            var mutableSummary = summary
            let options = occupiedOptions(from: &mutableSummary)
            return ShellPartyAssemblyPresentation(
                canAssemble: !options.isEmpty,
                hasActiveParty: mutableSummary.party_size > 0,
                message: options.isEmpty ? "Roster has no occupied slots" : "Select one to four party members",
                options: options
            )
        } ?? ShellPartyAssemblyPresentation(
            canAssemble: false,
            hasActiveParty: false,
            message: "Party assembly invalid current save",
            options: []
        )
    }

    func assemble(selectedRosterIDs: [UInt8], currentDocument: Data?) -> ShellPartyAssemblyResult {
        guard let currentDocument else {
            return ShellPartyAssemblyResult(assembled: false, message: "Party assembly requires current save", document: nil, activeRosterIDs: [])
        }

        guard selectedRosterIDs.count >= 1 && selectedRosterIDs.count <= Int(U3_PARTY_ACTIVE_SLOT_COUNT) else {
            return ShellPartyAssemblyResult(assembled: false, message: "Select one to four members", document: nil, activeRosterIDs: [])
        }

        var updatedDocument = currentDocument
        let updatedDocumentLength = updatedDocument.count
        let selectedCount = selectedRosterIDs.count
        var assembledRosterIDs: [UInt8] = []
        var failureMessage = "Party assembly failed"
        let assembled = updatedDocument.withUnsafeMutableBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                failureMessage = "Party assembly invalid document"
                return false
            }

            var document = u3_save_document()
            guard u3_save_open(documentBaseAddress, updatedDocumentLength, &document) != 0 else {
                failureMessage = "Party assembly invalid document"
                return false
            }

            var domainState = u3_save_domain_state()
            guard u3_save_load_domain_state(&document, &domainState) != 0 else {
                failureMessage = "Party assembly invalid document"
                return false
            }

            var partyRecord = u3_save_record()
            guard u3_save_find_record(&document, fourCharacterCode("PRTY"), Int16(U3_SAVE_ID_PARTY), &partyRecord) != 0,
                  partyRecord.length == U3_SAVE_PARTY_LENGTH,
                  let party = UnsafeMutableRawPointer(mutating: partyRecord.data)?.assumingMemoryBound(to: UInt8.self) else {
                failureMessage = "Party assembly missing party"
                return false
            }

            let result = selectedRosterIDs.withUnsafeBufferPointer { selectedBuffer -> u3_party_form_result in
                guard let selectedBaseAddress = selectedBuffer.baseAddress else {
                    return u3_party_form_result(formed: 0, reason: UInt8(U3_PARTY_FORM_INVALID_SIZE), party_size: 0, active_roster_ids: (0, 0, 0, 0))
                }

                return u3_party_form_from_roster(
                    &domainState,
                    selectedBaseAddress,
                    UInt8(selectedCount),
                    party,
                    UInt32(partyRecord.length)
                )
            }

            guard result.formed != 0 else {
                failureMessage = message(for: result.reason)
                return false
            }

            assembledRosterIDs = activeRosterIDs(from: result)
            return true
        }

        if !assembled {
            return ShellPartyAssemblyResult(assembled: false, message: failureMessage, document: nil, activeRosterIDs: [])
        }

        let activeDescription = assembledRosterIDs.map(String.init).joined(separator: "/")
        return ShellPartyAssemblyResult(
            assembled: true,
            message: "Party assembled \(activeDescription)",
            document: updatedDocument,
            activeRosterIDs: assembledRosterIDs
        )
    }

    private func loadSummary<T>(_ documentData: Data, body: (u3_party_summary) -> T) -> T? {
        documentData.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return nil
            }

            var document = u3_save_document()
            guard u3_save_open(baseAddress, documentData.count, &document) != 0 else {
                return nil
            }

            var state = u3_save_domain_state()
            guard u3_save_load_domain_state(&document, &state) != 0 else {
                return nil
            }

            var summary = u3_party_summary()
            guard u3_party_load_summary(&state, &summary) != 0 else {
                return nil
            }

            return body(summary)
        }
    }

    private func occupiedOptions(from summary: inout u3_party_summary) -> [ShellPartyRosterOption] {
        withUnsafePointer(to: &summary.roster) { pointer in
            pointer.withMemoryRebound(to: u3_party_roster_entry.self, capacity: Int(U3_PARTY_ROSTER_SLOT_COUNT)) { roster in
                (0..<Int(U3_PARTY_ROSTER_SLOT_COUNT)).compactMap { index in
                    let entry = roster[index]
                    guard entry.occupied != 0 else {
                        return nil
                    }

                    return ShellPartyRosterOption(
                        id: entry.roster_id,
                        name: stringFromPartyName(entry.name),
                        detail: "\(characterCode(entry.status)) \(characterCode(entry.race))/\(characterCode(entry.character_class))/\(characterCode(entry.sex)) HP \(entry.hit_points)/\(entry.max_hit_points) L\(entry.level)"
                    )
                }
            }
        }
    }

    private func activeRosterIDs(from summary: inout u3_party_summary) -> [UInt8] {
        let ids = [
            summary.active_roster_ids.0,
            summary.active_roster_ids.1,
            summary.active_roster_ids.2,
            summary.active_roster_ids.3
        ]
        return Array(ids.prefix(Int(summary.party_size)))
    }

    private func activeRosterIDs(from result: u3_party_form_result) -> [UInt8] {
        let ids = [
            result.active_roster_ids.0,
            result.active_roster_ids.1,
            result.active_roster_ids.2,
            result.active_roster_ids.3
        ]
        return Array(ids.prefix(Int(result.party_size)))
    }

    private func message(for reason: UInt8) -> String {
        switch Int32(reason) {
        case U3_PARTY_FORM_INVALID_ARGUMENT:
            return "Party assembly invalid request"
        case U3_PARTY_FORM_INVALID_PARTY:
            return "Party assembly invalid party"
        case U3_PARTY_FORM_INVALID_ROSTER:
            return "Party assembly invalid roster"
        case U3_PARTY_FORM_INVALID_SIZE:
            return "Select one to four members"
        case U3_PARTY_FORM_INVALID_ROSTER_ID:
            return "Party assembly invalid roster slot"
        case U3_PARTY_FORM_DUPLICATE_ROSTER_ID:
            return "Party assembly has duplicate members"
        case U3_PARTY_FORM_UNOCCUPIED_ROSTER_ID:
            return "Party assembly selected an empty slot"
        default:
            return "Party assembly failed"
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
}
