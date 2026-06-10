import Foundation
import Ultima3Core

struct ShellCharacterDraft {
    var name: String
    var race: UInt8
    var characterClass: UInt8
    var sex: UInt8
    var strength: UInt8
    var dexterity: UInt8
    var intelligence: UInt8
    var wisdom: UInt8
}

struct ShellCharacterValidationResult {
    let valid: Bool
    let reason: UInt8
    let message: String
    let summary: String
    let pointsRemaining: UInt8
    let pointsOver: UInt8
    let pointStatus: String
}

struct ShellCharacterPersistenceResult {
    let saved: Bool
    let message: String
    let document: Data?
    let rosterID: UInt8
}

final class ShellCharacterCreationAdapter {
    func validate(_ draft: ShellCharacterDraft) -> ShellCharacterValidationResult {
        var candidate = makeCandidate(from: draft)
        let validation = u3_character_validate_candidate(&candidate)
        let valid = validation.valid != 0
        let summary = summary(for: draft)
        return ShellCharacterValidationResult(
            valid: valid,
            reason: validation.reason,
            message: message(for: validation.reason, pointsRemaining: validation.points_remaining, pointsOver: validation.points_over),
            summary: summary,
            pointsRemaining: validation.points_remaining,
            pointsOver: validation.points_over,
            pointStatus: pointStatus(pointsRemaining: validation.points_remaining, pointsOver: validation.points_over)
        )
    }

    func persist(_ draft: ShellCharacterDraft, currentDocument: Data?) -> ShellCharacterPersistenceResult {
        guard let currentDocument else {
            return ShellCharacterPersistenceResult(saved: false, message: "Character requires current save", document: nil, rosterID: 0)
        }

        var candidate = makeCandidate(from: draft)
        var updatedDocument = currentDocument
        let updatedDocumentLength = updatedDocument.count
        var persistedRosterID: UInt8 = 0
        var failureMessage = "Character save failed"
        let saved = updatedDocument.withUnsafeMutableBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                failureMessage = "Character save invalid document"
                return false
            }

            var document = u3_save_document()
            guard u3_save_open(documentBaseAddress, updatedDocumentLength, &document) != 0 else {
                failureMessage = "Character save invalid document"
                return false
            }

            var domainState = u3_save_domain_state()
            guard u3_save_load_domain_state(&document, &domainState) != 0 else {
                failureMessage = "Character save invalid document"
                return false
            }

            var rosterRecord = u3_save_record()
            guard u3_save_find_record(&document, fourCharacterCode("ROST"), Int16(U3_SAVE_ID_ROSTER), &rosterRecord) != 0,
                  rosterRecord.length == U3_SAVE_ROSTER_LENGTH,
                  let roster = UnsafeMutableRawPointer(mutating: rosterRecord.data)?.assumingMemoryBound(to: UInt8.self) else {
                failureMessage = "Character save missing roster"
                return false
            }

            let write = u3_character_add_to_roster(&candidate, roster, Int(rosterRecord.length))
            guard write.written != 0 else {
                failureMessage = rosterWriteMessage(reason: write.reason, validationReason: write.validation.reason)
                return false
            }

            persistedRosterID = write.roster_id
            return true
        }

        if !saved {
            return ShellCharacterPersistenceResult(saved: false, message: failureMessage, document: nil, rosterID: 0)
        }

        return ShellCharacterPersistenceResult(
            saved: true,
            message: "\(summary(for: draft)) saved roster \(persistedRosterID)",
            document: updatedDocument,
            rosterID: persistedRosterID
        )
    }

    private func makeCandidate(from draft: ShellCharacterDraft) -> u3_character_candidate {
        var candidate = u3_character_candidate()
        populateName(draft.name, candidate: &candidate)
        candidate.race = draft.race
        candidate.character_class = draft.characterClass
        candidate.sex = draft.sex
        candidate.strength = draft.strength
        candidate.dexterity = draft.dexterity
        candidate.intelligence = draft.intelligence
        candidate.wisdom = draft.wisdom
        return candidate
    }

    private func summary(for draft: ShellCharacterDraft) -> String {
        "Character Candidate \(draft.name) \(characterCode(draft.sex))/\(characterCode(draft.race))/\(characterCode(draft.characterClass)) STR \(draft.strength) DEX \(draft.dexterity) INT \(draft.intelligence) WIS \(draft.wisdom)"
    }

    private func populateName(_ name: String, candidate: inout u3_character_candidate) {
        let bytes = Array(name.utf8.prefix(Int(U3_CHARACTER_NAME_CAPACITY) + 1))

        withUnsafeMutablePointer(to: &candidate.name) { pointer in
            pointer.withMemoryRebound(to: CChar.self, capacity: Int(U3_CHARACTER_NAME_CAPACITY) + 1) { characters in
                for index in 0...Int(U3_CHARACTER_NAME_CAPACITY) {
                    characters[index] = 0
                }

                for (index, byte) in bytes.enumerated() {
                    characters[index] = CChar(bitPattern: byte)
                }
            }
        }
    }

    private func message(for reason: UInt8, pointsRemaining: UInt8, pointsOver: UInt8) -> String {
        switch Int32(reason) {
        case U3_CHARACTER_VALIDATION_OK:
            return "Candidate OK"
        case U3_CHARACTER_VALIDATION_MISSING_NAME:
            return "Name required"
        case U3_CHARACTER_VALIDATION_NAME_TOO_LONG:
            return "Name must be 12 characters or fewer"
        case U3_CHARACTER_VALIDATION_INVALID_NAME_BYTE:
            return "Name uses unsupported characters"
        case U3_CHARACTER_VALIDATION_INVALID_RACE:
            return "Choose a valid race"
        case U3_CHARACTER_VALIDATION_INVALID_CLASS:
            return "Choose a valid class"
        case U3_CHARACTER_VALIDATION_INVALID_SEX:
            return "Choose a valid sex"
        case U3_CHARACTER_VALIDATION_ATTRIBUTE_RANGE:
            return "Attributes must be 5 through 25"
        case U3_CHARACTER_VALIDATION_ATTRIBUTE_TOTAL:
            if pointsOver > 0 {
                return "Use exactly 50 attribute points (\(pointsOver) over)"
            }
            return "Use exactly 50 attribute points (\(pointsRemaining) remaining)"
        default:
            return "Candidate invalid"
        }
    }

    private func pointStatus(pointsRemaining: UInt8, pointsOver: UInt8) -> String {
        if pointsOver > 0 {
            return "\(pointsOver) over"
        }

        return "\(pointsRemaining) pts"
    }

    private func rosterWriteMessage(reason: UInt8, validationReason: UInt8) -> String {
        switch Int32(reason) {
        case U3_CHARACTER_ROSTER_WRITE_INVALID_CANDIDATE:
            return message(for: validationReason, pointsRemaining: 0, pointsOver: 0)
        case U3_CHARACTER_ROSTER_WRITE_INVALID_ROSTER:
            return "Character save invalid roster"
        case U3_CHARACTER_ROSTER_WRITE_FULL:
            return "Roster full"
        default:
            return "Character save failed"
        }
    }

    private func fourCharacterCode(_ value: String) -> UInt32 {
        var result: UInt32 = 0

        for byte in value.utf8.prefix(4) {
            result = (result << 8) | UInt32(byte)
        }

        return result
    }

    private func characterCode(_ value: UInt8) -> String {
        guard value >= 32 && value <= 126,
              let scalar = UnicodeScalar(Int(value)) else {
            return "#\(value)"
        }

        return String(Character(scalar))
    }
}
