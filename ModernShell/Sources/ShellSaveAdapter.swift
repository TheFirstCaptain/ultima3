import Foundation
import Ultima3Core

final class ShellSaveAdapter {
    private let fileManager: FileManager

    init(fileManager: FileManager = .default) {
        self.fileManager = fileManager
    }

    func writeSmokeDocument(_ document: Data, saveDocumentPath: String) -> String {
        guard validateSaveDocument(document) else {
            return "Save Rejected invalid smoke document"
        }

        let destinationURL = URL(fileURLWithPath: saveDocumentPath, isDirectory: false)
        let directoryURL = destinationURL.deletingLastPathComponent()
        let temporaryURL = directoryURL.appendingPathComponent(".\(destinationURL.lastPathComponent).\(UUID().uuidString).tmp", isDirectory: false)

        do {
            try fileManager.createDirectory(at: directoryURL, withIntermediateDirectories: true)
            if isDirectory(destinationURL) {
                return "Save Failed Smoke"
            }
            try document.write(to: temporaryURL, options: .withoutOverwriting)

            let stagedData = try Data(contentsOf: temporaryURL)
            guard validateSaveDocument(stagedData) else {
                try? fileManager.removeItem(at: temporaryURL)
                return "Save Rejected staged smoke document"
            }

            if fileManager.fileExists(atPath: destinationURL.path) {
                _ = try fileManager.replaceItemAt(destinationURL, withItemAt: temporaryURL)
            } else {
                try fileManager.moveItem(at: temporaryURL, to: destinationURL)
            }

            guard let savedData = try? Data(contentsOf: destinationURL),
                  validateSaveDocument(savedData) else {
                return "Save Failed readback"
            }

            return "Save OK Smoke"
        } catch {
            try? fileManager.removeItem(at: temporaryURL)
            return "Save Failed Smoke"
        }
    }

    func readSmokeDocument(saveDocumentPath: String) -> String {
        let destinationURL = URL(fileURLWithPath: saveDocumentPath, isDirectory: false)

        guard let data = try? Data(contentsOf: destinationURL) else {
            return "Save Missing Smoke"
        }

        return validateSaveDocument(data) ? "Save Read OK Smoke" : "Save Read Invalid Smoke"
    }

    func readDocument(saveDocumentPath: String) -> Data? {
        let destinationURL = URL(fileURLWithPath: saveDocumentPath, isDirectory: false)

        guard let data = try? Data(contentsOf: destinationURL),
              validateSaveDocument(data) else {
            return nil
        }

        return data
    }

    private func validateSaveDocument(_ document: Data) -> Bool {
        document.withUnsafeBytes { rawBuffer in
            guard let baseAddress = rawBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }

            var saveDocument = u3_save_document()
            return u3_save_open(baseAddress, document.count, &saveDocument) != 0
        }
    }

    private func isDirectory(_ url: URL) -> Bool {
        var isDirectory: ObjCBool = false
        return fileManager.fileExists(atPath: url.path, isDirectory: &isDirectory) && isDirectory.boolValue
    }
}
