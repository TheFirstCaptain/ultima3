import Foundation

struct ShellLocationSnapshot {
    let resourceRootPath: String?
    let saveDocumentPath: String

    var resourceStatus: String {
        guard let resourceRootPath else {
            return "Resource root missing"
        }

        return "Resource root \(resourceRootPath)"
    }

    var saveStatus: String {
        "Save path \(saveDocumentPath)"
    }
}

final class ShellLocationProvider {
    private let fileManager: FileManager

    init(fileManager: FileManager = .default) {
        self.fileManager = fileManager
    }

    func snapshot() -> ShellLocationSnapshot {
        ShellLocationSnapshot(
            resourceRootPath: resolveResourceRoot()?.path,
            saveDocumentPath: resolveSaveDocumentPath().path
        )
    }

    private func resolveResourceRoot() -> URL? {
        let currentDirectory = URL(fileURLWithPath: fileManager.currentDirectoryPath, isDirectory: true)
        let candidates = [
            currentDirectory.appendingPathComponent("Resources", isDirectory: true),
            currentDirectory.deletingLastPathComponent().appendingPathComponent("Resources", isDirectory: true),
            Bundle.main.resourceURL?.appendingPathComponent("Resources", isDirectory: true)
        ].compactMap { $0 }

        return candidates.first { isDirectory($0) }
    }

    private func resolveSaveDocumentPath() -> URL {
        if let applicationSupport = fileManager.urls(for: .applicationSupportDirectory, in: .userDomainMask).first {
            return applicationSupport
                .appendingPathComponent("Ultima3Modernization", isDirectory: true)
                .appendingPathComponent("Ultima3ModernSave.u3save", isDirectory: false)
        }

        let currentDirectory = URL(fileURLWithPath: fileManager.currentDirectoryPath, isDirectory: true)
        return currentDirectory
            .appendingPathComponent("Ultima3ModernSave.u3save", isDirectory: false)
    }

    private func isDirectory(_ url: URL) -> Bool {
        var isDirectory: ObjCBool = false
        return fileManager.fileExists(atPath: url.path, isDirectory: &isDirectory) && isDirectory.boolValue
    }
}
