import Foundation
import Ultima3Core

enum ShellSaveWriteResult: Equatable {
    case saved
    case rejectedInvalidDocument
    case rejectedStagedDocument
    case failed
    case failedReadback

    var message: String {
        switch self {
        case .saved:
            return "Game saved"
        case .rejectedInvalidDocument:
            return "Save rejected: invalid document"
        case .rejectedStagedDocument:
            return "Save rejected: staged document invalid"
        case .failed:
            return "Save failed"
        case .failedReadback:
            return "Save failed: readback mismatch"
        }
    }
}

final class ShellSaveAdapter {
    private let fileManager: FileManager
    private let readback: (URL) -> Data?

    init(
        fileManager: FileManager = .default,
        readback: @escaping (URL) -> Data? = { try? Data(contentsOf: $0) }
    ) {
        self.fileManager = fileManager
        self.readback = readback
    }

    func writeDocument(_ document: Data, saveDocumentPath: String) -> ShellSaveWriteResult {
        guard validateSaveDocument(document) else {
            return .rejectedInvalidDocument
        }

        let destinationURL = URL(fileURLWithPath: saveDocumentPath, isDirectory: false)
        let directoryURL = destinationURL.deletingLastPathComponent()
        let temporaryURL = directoryURL.appendingPathComponent(".\(destinationURL.lastPathComponent).\(UUID().uuidString).tmp", isDirectory: false)
        let backupName = ".\(destinationURL.lastPathComponent).\(UUID().uuidString).backup"
        let backupURL = directoryURL.appendingPathComponent(backupName, isDirectory: false)

        do {
            try fileManager.createDirectory(at: directoryURL, withIntermediateDirectories: true)
            if isDirectory(destinationURL) {
                return .failed
            }
            try document.write(to: temporaryURL, options: .withoutOverwriting)

            let stagedData = try Data(contentsOf: temporaryURL)
            guard validateSaveDocument(stagedData), stagedData == document else {
                try? fileManager.removeItem(at: temporaryURL)
                return .rejectedStagedDocument
            }

            if fileManager.fileExists(atPath: destinationURL.path) {
                _ = try fileManager.replaceItemAt(
                    destinationURL,
                    withItemAt: temporaryURL,
                    backupItemName: backupName,
                    options: [.withoutDeletingBackupItem]
                )
            } else {
                try fileManager.moveItem(at: temporaryURL, to: destinationURL)
            }

            guard let savedData = readback(destinationURL),
                  validateSaveDocument(savedData),
                  savedData == document else {
                restoreBackupIfPresent(backupURL, destinationURL: destinationURL)
                return .failedReadback
            }

            try? fileManager.removeItem(at: backupURL)
            return .saved
        } catch {
            try? fileManager.removeItem(at: temporaryURL)
            restoreBackupIfPresent(backupURL, destinationURL: destinationURL)
            return .failed
        }
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

    private func restoreBackupIfPresent(_ backupURL: URL, destinationURL: URL) {
        guard fileManager.fileExists(atPath: backupURL.path) else {
            return
        }

        do {
            if fileManager.fileExists(atPath: destinationURL.path) {
                _ = try fileManager.replaceItemAt(destinationURL, withItemAt: backupURL)
            } else {
                try fileManager.moveItem(at: backupURL, to: destinationURL)
            }
        } catch {
            // Keep the backup in place when restoration cannot complete.
        }
    }
}
