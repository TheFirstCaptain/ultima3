// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "Ultima3Modernization",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .executable(
            name: "Ultima3ModernShell",
            targets: ["Ultima3ModernShell"]
        ),
        .library(
            name: "Ultima3Core",
            targets: ["Ultima3Core"]
        )
    ],
    targets: [
        .target(
            name: "Ultima3Core",
            path: "Core",
            sources: ["src"],
            publicHeadersPath: "include"
        ),
        .executableTarget(
            name: "Ultima3ModernShell",
            dependencies: ["Ultima3Core"],
            path: "ModernShell/Sources"
        ),
        .testTarget(
            name: "Ultima3ModernShellTests",
            dependencies: ["Ultima3ModernShell", "Ultima3Core"],
            path: "ModernShell/Tests"
        )
    ]
)
