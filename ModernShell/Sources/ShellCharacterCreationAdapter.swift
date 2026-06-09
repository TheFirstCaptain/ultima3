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

final class ShellCharacterCreationAdapter {
    func validate(_ draft: ShellCharacterDraft) -> ShellCharacterValidationResult {
        var candidate = u3_character_candidate()
        populateName(draft.name, candidate: &candidate)
        candidate.race = draft.race
        candidate.character_class = draft.characterClass
        candidate.sex = draft.sex
        candidate.strength = draft.strength
        candidate.dexterity = draft.dexterity
        candidate.intelligence = draft.intelligence
        candidate.wisdom = draft.wisdom

        let validation = u3_character_validate_candidate(&candidate)
        let valid = validation.valid != 0
        let summary = "Character Candidate \(draft.name) \(characterCode(draft.sex))/\(characterCode(draft.race))/\(characterCode(draft.characterClass)) STR \(draft.strength) DEX \(draft.dexterity) INT \(draft.intelligence) WIS \(draft.wisdom)"
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

    private func characterCode(_ value: UInt8) -> String {
        guard value >= 32 && value <= 126,
              let scalar = UnicodeScalar(Int(value)) else {
            return "#\(value)"
        }

        return String(Character(scalar))
    }
}
