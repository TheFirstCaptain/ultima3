import XCTest
@testable import Ultima3ModernShell

final class ShellCharacterCreationAdapterTests: XCTestCase {
    func testValidCandidateProducesSummary() {
        let adapter = ShellCharacterCreationAdapter()

        let result = adapter.validate(ShellCharacterDraft(
            name: "Iolo",
            race: UInt8(ascii: "H"),
            characterClass: UInt8(ascii: "F"),
            sex: UInt8(ascii: "M"),
            strength: 15,
            dexterity: 15,
            intelligence: 10,
            wisdom: 10
        ))

        XCTAssertTrue(result.valid)
        XCTAssertEqual(result.message, "Candidate OK")
        XCTAssertEqual(result.pointsRemaining, 0)
        XCTAssertEqual(result.pointsOver, 0)
        XCTAssertEqual(result.pointStatus, "0 pts")
        XCTAssertEqual(result.summary, "Character Candidate Iolo M/H/F STR 15 DEX 15 INT 10 WIS 10")
    }

    func testBarbarianCandidateIsValid() {
        let adapter = ShellCharacterCreationAdapter()

        let result = adapter.validate(ShellCharacterDraft(
            name: "Dupre",
            race: UInt8(ascii: "H"),
            characterClass: UInt8(ascii: "B"),
            sex: UInt8(ascii: "M"),
            strength: 20,
            dexterity: 10,
            intelligence: 10,
            wisdom: 10
        ))

        XCTAssertTrue(result.valid)
        XCTAssertEqual(result.summary, "Character Candidate Dupre M/H/B STR 20 DEX 10 INT 10 WIS 10")
    }

    func testInvalidCandidateReportsPortableValidationMessage() {
        let adapter = ShellCharacterCreationAdapter()

        let result = adapter.validate(ShellCharacterDraft(
            name: "",
            race: UInt8(ascii: "H"),
            characterClass: UInt8(ascii: "F"),
            sex: UInt8(ascii: "M"),
            strength: 15,
            dexterity: 15,
            intelligence: 10,
            wisdom: 10
        ))

        XCTAssertFalse(result.valid)
        XCTAssertEqual(result.message, "Name required")
    }

    func testOverspentAttributesReportOverage() {
        let adapter = ShellCharacterCreationAdapter()

        let result = adapter.validate(ShellCharacterDraft(
            name: "Iolo",
            race: UInt8(ascii: "H"),
            characterClass: UInt8(ascii: "F"),
            sex: UInt8(ascii: "M"),
            strength: 16,
            dexterity: 15,
            intelligence: 10,
            wisdom: 10
        ))

        XCTAssertFalse(result.valid)
        XCTAssertEqual(result.message, "Use exactly 50 attribute points (1 over)")
        XCTAssertEqual(result.pointsOver, 1)
        XCTAssertEqual(result.pointStatus, "1 over")
    }

    func testShellStateRecordsAcceptedAndCancelledCandidate() {
        let state = ShellSmokeState()

        state.acceptCharacterCandidate("Character Candidate Iolo M/H/F STR 15 DEX 15 INT 10 WIS 10")
        XCTAssertEqual(state.lastCommand, "Character candidate accepted")
        XCTAssertEqual(state.saveStatus, "Character Candidate Iolo M/H/F STR 15 DEX 15 INT 10 WIS 10")

        state.cancelCharacterCandidate()
        XCTAssertEqual(state.lastCommand, "Character creation cancelled")
        XCTAssertEqual(state.saveStatus, "Character Candidate Iolo M/H/F STR 15 DEX 15 INT 10 WIS 10")
    }
}
