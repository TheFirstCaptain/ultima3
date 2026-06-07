import AppKit
import SwiftUI
import Ultima3Core

final class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?
    private var preferencesWindowController: NSWindowController?
    private let shellState = ShellSmokeState()

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.mainMenu = makeMainMenu()
        openMainWindow()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        true
    }

    @objc private func newGame(_ sender: Any?) {
        shellState.submitMenuCommand(U3_INPUT_MENU_NEW_GAME)
    }

    @objc private func saveGame(_ sender: Any?) {
        shellState.submitMenuCommand(U3_INPUT_MENU_SAVE)
    }

    @objc private func testSound(_ sender: Any?) {
        shellState.submitAudioSound(U3_AUDIO_SOUND_ERROR1)
    }

    @objc private func refreshLocations(_ sender: Any?) {
        shellState.refreshLocationStatus()
    }

    @objc private func writeSaveSmoke(_ sender: Any?) {
        shellState.writeSaveSmoke()
    }

    @objc private func loadNewGameSmoke(_ sender: Any?) {
        shellState.loadNewGameSmoke()
    }

    @objc private func showPreferences(_ sender: Any?) {
        if preferencesWindowController == nil {
            let hostingController = NSHostingController(rootView: ModernShellPreferencesView())
            let panel = NSWindow(contentViewController: hostingController)
            panel.title = "Preferences"
            panel.styleMask = [.titled, .closable]
            panel.setContentSize(NSSize(width: 360, height: 220))
            preferencesWindowController = NSWindowController(window: panel)
        }

        preferencesWindowController?.showWindow(sender)
        NSApp.activate(ignoringOtherApps: true)
    }

    private func openMainWindow() {
        let gameHostView = GameHostView(shellState: shellState)
        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 960, height: 640),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.title = "Ultima III Modern Shell"
        window.contentView = gameHostView
        window.center()
        window.makeKeyAndOrderFront(nil)
        window.makeFirstResponder(gameHostView)
        NSApp.activate(ignoringOtherApps: true)
        self.window = window
    }

    private func makeMainMenu() -> NSMenu {
        let mainMenu = NSMenu()

        let appMenuItem = NSMenuItem()
        let appMenu = NSMenu()
        appMenu.addItem(makeMenuItem(
            withTitle: "Preferences...",
            action: #selector(showPreferences(_:)),
            keyEquivalent: ",",
            target: self
        ))
        appMenu.addItem(NSMenuItem.separator())
        appMenu.addItem(makeMenuItem(
            withTitle: "Quit Ultima III Modern Shell",
            action: #selector(NSApplication.terminate(_:)),
            keyEquivalent: "q",
            target: NSApp
        ))
        appMenuItem.submenu = appMenu
        mainMenu.addItem(appMenuItem)

        let gameMenuItem = NSMenuItem()
        let gameMenu = NSMenu(title: "Game")
        gameMenu.addItem(makeMenuItem(
            withTitle: "New Game",
            action: #selector(newGame(_:)),
            keyEquivalent: "n",
            target: self
        ))
        gameMenu.addItem(makeMenuItem(
            withTitle: "Save",
            action: #selector(saveGame(_:)),
            keyEquivalent: "s",
            target: self
        ))
        gameMenu.addItem(makeMenuItem(
            withTitle: "Refresh Locations",
            action: #selector(refreshLocations(_:)),
            keyEquivalent: "l",
            target: self
        ))
        gameMenu.addItem(makeMenuItem(
            withTitle: "Write Save Smoke",
            action: #selector(writeSaveSmoke(_:)),
            keyEquivalent: "w",
            target: self
        ))
        gameMenu.addItem(makeMenuItem(
            withTitle: "Load New Game Smoke",
            action: #selector(loadNewGameSmoke(_:)),
            keyEquivalent: "d",
            target: self
        ))
        gameMenu.addItem(NSMenuItem.separator())
        gameMenu.addItem(makeMenuItem(
            withTitle: "Test Sound",
            action: #selector(testSound(_:)),
            keyEquivalent: "t",
            target: self
        ))
        gameMenuItem.submenu = gameMenu
        mainMenu.addItem(gameMenuItem)

        return mainMenu
    }

    private func makeMenuItem(
        withTitle title: String,
        action: Selector,
        keyEquivalent: String,
        target: AnyObject
    ) -> NSMenuItem {
        let item = NSMenuItem(title: title, action: action, keyEquivalent: keyEquivalent)
        item.target = target
        return item
    }
}

final class ShellSmokeState: ObservableObject {
    @Published private(set) var lastCommand = "Ready"
    @Published private(set) var resourceStatus: String
    @Published private(set) var saveStatus: String
    @Published private(set) var renderFrame = u3_render_make_synthetic_tile_frame()

    private let inputAdapter = ShellInputAdapter()
    private let audioAdapter = ShellAudioAdapter()
    private let locationProvider = ShellLocationProvider()
    private let resourceAdapter = ShellResourceAdapter()
    private let saveAdapter = ShellSaveAdapter()
    let coreHeadingProbe: Int8 = u3_map_math_get_heading(1)

    init() {
        let locations = locationProvider.snapshot()
        let renderSmoke = resourceAdapter.buildResourceBackedRenderSmokeFrame(resourceRootPath: locations.resourceRootPath)
        renderFrame = renderSmoke.frame
        resourceStatus = Self.describeResourceStatus(
            locations: locations,
            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
            renderStatus: renderSmoke.status
        )
        saveStatus = locations.saveStatus
    }

    func submitKeyboard(_ key: UInt8) {
        lastCommand = inputAdapter.submitKeyboard(key)
    }

    func submitMouseDown(x: Int16, y: Int16) {
        lastCommand = inputAdapter.submitMouseDown(x: x, y: y)
    }

    func submitMenuCommand(_ command: Int32) {
        lastCommand = inputAdapter.submitMenuCommand(command)
    }

    func submitAudioSound(_ sound: Int32) {
        lastCommand = audioAdapter.submitSound(sound)
    }

    func refreshLocationStatus() {
        let locations = locationProvider.snapshot()
        let renderSmoke = resourceAdapter.buildResourceBackedRenderSmokeFrame(resourceRootPath: locations.resourceRootPath)
        renderFrame = renderSmoke.frame
        resourceStatus = Self.describeResourceStatus(
            locations: locations,
            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
            renderStatus: renderSmoke.status
        )
        saveStatus = locations.saveStatus
        lastCommand = "Locations refreshed"
    }

    func writeSaveSmoke() {
        let locations = locationProvider.snapshot()
        guard let document = resourceAdapter.buildNativeNewGameSmokeDocument(resourceRootPath: locations.resourceRootPath) else {
            saveStatus = "Save Failed build smoke document"
            lastCommand = "Save smoke failed"
            return
        }

        let writeStatus = saveAdapter.writeSmokeDocument(document, saveDocumentPath: locations.saveDocumentPath)
        let readStatus = saveAdapter.readSmokeDocument(saveDocumentPath: locations.saveDocumentPath)
        saveStatus = "\(writeStatus) | \(readStatus) | \(locations.saveDocumentPath)"
        lastCommand = "Save smoke"
    }

    func loadNewGameSmoke() {
        let locations = locationProvider.snapshot()
        saveStatus = resourceAdapter.loadNewGameSmokeState(resourceRootPath: locations.resourceRootPath)
        lastCommand = "New game smoke"
    }

    private static func describeResourceStatus(locations: ShellLocationSnapshot, validation: String, renderStatus: String) -> String {
        guard let resourceRootPath = locations.resourceRootPath else {
            return "\(validation) | \(renderStatus)"
        }

        return "\(validation) | \(renderStatus) | root \(resourceRootPath)"
    }
}
