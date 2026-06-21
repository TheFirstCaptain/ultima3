import XCTest
@testable import Ultima3ModernShell

final class ShellCharacterCreationAdapterTests: XCTestCase {
    private var resourceRootPath: String {
        URL(fileURLWithPath: FileManager.default.currentDirectoryPath, isDirectory: true)
            .appendingPathComponent("Resources", isDirectory: true)
            .path
    }

    private var validDraft: ShellCharacterDraft {
        ShellCharacterDraft(
            name: "Iolo",
            race: UInt8(ascii: "H"),
            characterClass: UInt8(ascii: "F"),
            sex: UInt8(ascii: "M"),
            strength: 15,
            dexterity: 15,
            intelligence: 10,
            wisdom: 10
        )
    }

    func testValidCandidateProducesSummary() {
        let adapter = ShellCharacterCreationAdapter()

        let result = adapter.validate(validDraft)

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

        state.acceptCharacterCandidate(validDraft)
        XCTAssertEqual(state.lastCommand, "Character save failed")
        XCTAssertEqual(state.saveStatus, "Character requires current save")

        state.newGame()
        state.acceptCharacterCandidate(validDraft)
        XCTAssertEqual(state.lastCommand, "Character saved roster 5")
        XCTAssertEqual(state.saveStatus, "Party OK size 4 active 1/2/3/4 occupied 5 lead Tatiana G E/T/F HP 100/100 L1 food 150 gold 150")

        state.cancelCharacterCandidate()
        XCTAssertEqual(state.lastCommand, "Character creation cancelled")
        XCTAssertEqual(state.saveStatus, "Party OK size 4 active 1/2/3/4 occupied 5 lead Tatiana G E/T/F HP 100/100 L1 food 150 gold 150")
    }

    func testPersistWritesCandidateIntoCurrentDocument() throws {
        let adapter = ShellCharacterCreationAdapter()
        let resourceAdapter = ShellResourceAdapter()
        let document = try XCTUnwrap(resourceAdapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))

        let result = adapter.persist(validDraft, currentDocument: document)

        XCTAssertTrue(result.saved)
        XCTAssertEqual(result.rosterID, 5)
        let updatedDocument = try XCTUnwrap(result.document)
        XCTAssertEqual(resourceAdapter.inspectPartyRoster(documentData: updatedDocument), "Party OK size 4 active 1/2/3/4 occupied 5 lead Tatiana G E/T/F HP 100/100 L1 food 150 gold 150")
    }

    func testPersistRejectsDomainInvalidDocumentBeforeMutation() throws {
        let adapter = ShellCharacterCreationAdapter()
        let resourceAdapter = ShellResourceAdapter()
        var document = try XCTUnwrap(resourceAdapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))
        let originalDocument = document

        writeUInt32(0x42414421, at: 64, into: &document)
        let result = adapter.persist(validDraft, currentDocument: document)

        XCTAssertFalse(result.saved)
        XCTAssertEqual(result.message, "Character save invalid document")
        XCTAssertNil(result.document)
        XCTAssertEqual(document.count, originalDocument.count)
    }

    private func writeUInt32(_ value: UInt32, at offset: Int, into data: inout Data) {
        data[offset] = UInt8(value >> 24)
        data[offset + 1] = UInt8((value >> 16) & 0xFF)
        data[offset + 2] = UInt8((value >> 8) & 0xFF)
        data[offset + 3] = UInt8(value & 0xFF)
    }
}
