import XCTest
@testable import Ultima3ModernShell

final class ShellPartyAssemblyAdapterTests: XCTestCase {
    private var resourceRootPath: String {
        URL(fileURLWithPath: FileManager.default.currentDirectoryPath, isDirectory: true)
            .appendingPathComponent("Resources", isDirectory: true)
            .path
    }

    func testPresentationRequiresCurrentDocument() {
        let adapter = ShellPartyAssemblyAdapter()

        let presentation = adapter.presentation(currentDocument: nil)

        XCTAssertFalse(presentation.canAssemble)
        XCTAssertFalse(presentation.hasActiveParty)
        XCTAssertEqual(presentation.message, "Party assembly requires current save")
        XCTAssertTrue(presentation.options.isEmpty)
    }

    func testPresentationListsOccupiedRosterEntries() throws {
        let adapter = ShellPartyAssemblyAdapter()
        let resourceAdapter = ShellResourceAdapter()
        let document = try XCTUnwrap(resourceAdapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))

        let presentation = adapter.presentation(currentDocument: document)

        XCTAssertTrue(presentation.canAssemble)
        XCTAssertTrue(presentation.hasActiveParty)
        XCTAssertEqual(presentation.message, "Select one to four party members")
        XCTAssertEqual(presentation.options.map(\.id), [1, 2, 3, 4])
        XCTAssertEqual(presentation.options.first?.name, "Tatiana")
    }

    func testAssembleWritesOrderedPartyIntoCurrentDocument() throws {
        let adapter = ShellPartyAssemblyAdapter()
        let resourceAdapter = ShellResourceAdapter()
        let document = try XCTUnwrap(resourceAdapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))

        let result = adapter.assemble(selectedRosterIDs: [2, 1], currentDocument: document)

        XCTAssertTrue(result.assembled)
        XCTAssertEqual(result.message, "Party assembled 2/1")
        XCTAssertEqual(result.activeRosterIDs, [2, 1])
        let updatedDocument = try XCTUnwrap(result.document)
        let inspection = resourceAdapter.inspectPartyRoster(documentData: updatedDocument)
        XCTAssertTrue(inspection.contains("Party OK size 2 active 2/1/0/0 occupied 4"))
    }

    func testAssembleRejectsInvalidSelectionWithoutDocument() {
        let adapter = ShellPartyAssemblyAdapter()

        let result = adapter.assemble(selectedRosterIDs: [1], currentDocument: nil)

        XCTAssertFalse(result.assembled)
        XCTAssertEqual(result.message, "Party assembly requires current save")
        XCTAssertNil(result.document)
    }

    func testAssembleRejectsInvalidSelectionCountsBeforeMutation() throws {
        let adapter = ShellPartyAssemblyAdapter()
        let resourceAdapter = ShellResourceAdapter()
        let document = try XCTUnwrap(resourceAdapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))

        let empty = adapter.assemble(selectedRosterIDs: [], currentDocument: document)
        XCTAssertFalse(empty.assembled)
        XCTAssertEqual(empty.message, "Select one to four members")
        XCTAssertNil(empty.document)

        let tooMany = adapter.assemble(selectedRosterIDs: [1, 2, 3, 4, 5], currentDocument: document)
        XCTAssertFalse(tooMany.assembled)
        XCTAssertEqual(tooMany.message, "Select one to four members")
        XCTAssertNil(tooMany.document)

        let oversized = adapter.assemble(selectedRosterIDs: Array(repeating: 1, count: 300), currentDocument: document)
        XCTAssertFalse(oversized.assembled)
        XCTAssertEqual(oversized.message, "Select one to four members")
        XCTAssertNil(oversized.document)
    }

    func testAssembleReportsPortableSelectionFailures() throws {
        let adapter = ShellPartyAssemblyAdapter()
        let resourceAdapter = ShellResourceAdapter()
        let document = try XCTUnwrap(resourceAdapter.buildNativeNewGameSmokeDocument(resourceRootPath: resourceRootPath))

        let duplicate = adapter.assemble(selectedRosterIDs: [1, 1], currentDocument: document)
        XCTAssertFalse(duplicate.assembled)
        XCTAssertEqual(duplicate.message, "Party assembly has duplicate members")
        XCTAssertNil(duplicate.document)

        let unoccupied = adapter.assemble(selectedRosterIDs: [5], currentDocument: document)
        XCTAssertFalse(unoccupied.assembled)
        XCTAssertEqual(unoccupied.message, "Party assembly selected an empty slot")
        XCTAssertNil(unoccupied.document)
    }

    func testShellStateRecordsAcceptedAndCancelledAssembly() {
        let state = ShellSmokeState()

        state.acceptPartyAssembly([1])
        XCTAssertEqual(state.lastCommand, "Party assembly failed")
        XCTAssertEqual(state.saveStatus, "Party assembly requires current save")

        state.newGame()
        state.acceptPartyAssembly([2, 1])
        XCTAssertEqual(state.lastCommand, "Party assembled 2/1")
        XCTAssertTrue(state.saveStatus.contains("Party OK size 2 active 2/1/0/0 occupied 4"))

        state.cancelPartyAssembly()
        XCTAssertEqual(state.lastCommand, "Party assembly cancelled")
    }
}
