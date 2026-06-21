import XCTest
@testable import Ultima3ModernShell

final class ShellSaveAdapterTests: XCTestCase {
    private var temporaryDirectory: URL!

    override func setUpWithError() throws {
        temporaryDirectory = FileManager.default.temporaryDirectory
            .appendingPathComponent("Ultima3ModernShellTests-\(UUID().uuidString)", isDirectory: true)
        try FileManager.default.createDirectory(at: temporaryDirectory, withIntermediateDirectories: true)
    }

    override func tearDownWithError() throws {
        if let temporaryDirectory {
            try? FileManager.default.removeItem(at: temporaryDirectory)
        }
    }

    func testWriteDocumentCreatesDirectoryAndRoundTripsValidDocument() throws {
        let adapter = ShellSaveAdapter()
        let saveURL = temporaryDirectory
            .appendingPathComponent("Nested", isDirectory: true)
            .appendingPathComponent("Smoke.u3save", isDirectory: false)
        let document = makeValidSaveDocument(payload: [1, 2, 3, 4])

        XCTAssertEqual(adapter.writeDocument(document, saveDocumentPath: saveURL.path), .saved)
        XCTAssertEqual(try Data(contentsOf: saveURL), document)
    }

    func testReadDocumentReturnsValidDocumentData() throws {
        let adapter = ShellSaveAdapter()
        let saveURL = temporaryDirectory.appendingPathComponent("Smoke.u3save", isDirectory: false)
        let document = makeValidSaveDocument(payload: [21, 22, 23, 24])

        XCTAssertEqual(adapter.writeDocument(document, saveDocumentPath: saveURL.path), .saved)
        XCTAssertEqual(adapter.readDocument(saveDocumentPath: saveURL.path), document)
    }

    func testReadDocumentRejectsInvalidDocumentData() throws {
        let adapter = ShellSaveAdapter()
        let saveURL = temporaryDirectory.appendingPathComponent("Invalid.u3save", isDirectory: false)
        var invalidDocument = makeValidSaveDocument(payload: [25, 26, 27, 28])
        invalidDocument[0] = UInt8(ascii: "X")
        try invalidDocument.write(to: saveURL)

        XCTAssertNil(adapter.readDocument(saveDocumentPath: saveURL.path))
    }

    func testInvalidReplacementRetainsPreviousValidDocument() throws {
        let adapter = ShellSaveAdapter()
        let saveURL = temporaryDirectory.appendingPathComponent("Smoke.u3save", isDirectory: false)
        let document = makeValidSaveDocument(payload: [5, 6, 7, 8])
        var invalidDocument = document
        invalidDocument[0] = UInt8(ascii: "X")

        XCTAssertEqual(adapter.writeDocument(document, saveDocumentPath: saveURL.path), .saved)
        XCTAssertEqual(adapter.writeDocument(invalidDocument, saveDocumentPath: saveURL.path), .rejectedInvalidDocument)
        XCTAssertEqual(try Data(contentsOf: saveURL), document)
    }

    func testValidReplacementOverwritesPreviousDocument() throws {
        let adapter = ShellSaveAdapter()
        let saveURL = temporaryDirectory.appendingPathComponent("Smoke.u3save", isDirectory: false)
        let firstDocument = makeValidSaveDocument(payload: [13, 14, 15, 16])
        let secondDocument = makeValidSaveDocument(payload: [17, 18, 19, 20])

        XCTAssertEqual(adapter.writeDocument(firstDocument, saveDocumentPath: saveURL.path), .saved)
        XCTAssertEqual(adapter.writeDocument(secondDocument, saveDocumentPath: saveURL.path), .saved)
        XCTAssertEqual(try Data(contentsOf: saveURL), secondDocument)
        XCTAssertEqual(
            try FileManager.default.contentsOfDirectory(atPath: temporaryDirectory.path)
                .filter { $0.hasSuffix(".backup") },
            []
        )
    }

    func testReadbackFailureRestoresPreviousDocument() throws {
        let saveURL = temporaryDirectory.appendingPathComponent("Smoke.u3save", isDirectory: false)
        let firstDocument = makeValidSaveDocument(payload: [31, 32, 33, 34])
        let secondDocument = makeValidSaveDocument(payload: [35, 36, 37, 38])
        XCTAssertEqual(
            ShellSaveAdapter().writeDocument(firstDocument, saveDocumentPath: saveURL.path),
            .saved
        )
        let failingReadbackAdapter = ShellSaveAdapter(readback: { _ in nil })

        XCTAssertEqual(
            failingReadbackAdapter.writeDocument(secondDocument, saveDocumentPath: saveURL.path),
            .failedReadback
        )
        XCTAssertEqual(try Data(contentsOf: saveURL), firstDocument)
        XCTAssertEqual(
            try FileManager.default.contentsOfDirectory(atPath: temporaryDirectory.path)
                .filter { $0.hasSuffix(".backup") || $0.hasSuffix(".tmp") },
            []
        )
    }

    func testFailedWriteCleansUniqueTemporaryFiles() throws {
        let adapter = ShellSaveAdapter()
        let directoryURL = temporaryDirectory.appendingPathComponent("DestinationAsDirectory", isDirectory: true)
        let saveURL = directoryURL
        let document = makeValidSaveDocument(payload: [9, 10, 11, 12])
        try FileManager.default.createDirectory(at: directoryURL, withIntermediateDirectories: true)

        XCTAssertEqual(adapter.writeDocument(document, saveDocumentPath: saveURL.path), .failed)

        let temporaryFiles = try FileManager.default.contentsOfDirectory(atPath: temporaryDirectory.path)
            .filter { $0.contains(".tmp") }
        XCTAssertTrue(temporaryFiles.isEmpty)
    }

    private func makeValidSaveDocument(payload: [UInt8]) -> Data {
        var data = Data(count: 16 + 16)
        data[0] = UInt8(ascii: "U")
        data[1] = UInt8(ascii: "3")
        data[2] = UInt8(ascii: "S")
        data[3] = UInt8(ascii: "V")
        writeUInt16(1, at: 4, into: &data)
        writeUInt16(1, at: 6, into: &data)
        writeUInt32(32, at: 8, into: &data)
        writeUInt32(0x4D455441, at: 16, into: &data)
        writeUInt16(1, at: 20, into: &data)
        writeUInt16(0, at: 22, into: &data)
        writeUInt32(32, at: 24, into: &data)
        writeUInt32(UInt32(payload.count), at: 28, into: &data)
        data.append(contentsOf: payload)
        return data
    }

    private func writeUInt16(_ value: UInt16, at offset: Int, into data: inout Data) {
        data[offset] = UInt8(value >> 8)
        data[offset + 1] = UInt8(value & 0xFF)
    }

    private func writeUInt32(_ value: UInt32, at offset: Int, into data: inout Data) {
        data[offset] = UInt8(value >> 24)
        data[offset + 1] = UInt8((value >> 16) & 0xFF)
        data[offset + 2] = UInt8((value >> 8) & 0xFF)
        data[offset + 3] = UInt8(value & 0xFF)
    }
}
