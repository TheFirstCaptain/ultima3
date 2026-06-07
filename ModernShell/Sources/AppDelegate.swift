import AppKit
import SwiftUI
import Ultima3Core

final class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?
    private var preferencesWindowController: NSWindowController?
    private var tickTimer: Timer?
    private let shellState = ShellSmokeState()

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.mainMenu = makeMainMenu()
        openMainWindow()
        startTickTimer()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        true
    }

    func applicationWillTerminate(_ notification: Notification) {
        tickTimer?.invalidate()
        tickTimer = nil
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

    @objc private func toggleTick(_ sender: Any?) {
        if tickTimer == nil {
            startTickTimer()
            shellState.setTickActive(true)
        } else {
            tickTimer?.invalidate()
            tickTimer = nil
            shellState.setTickActive(false)
        }
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

    @objc private func inspectPartyRoster(_ sender: Any?) {
        shellState.inspectPartyRoster()
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
        gameMenu.addItem(makeMenuItem(
            withTitle: "Inspect Party/Roster",
            action: #selector(inspectPartyRoster(_:)),
            keyEquivalent: "i",
            target: self
        ))
        gameMenu.addItem(NSMenuItem.separator())
        gameMenu.addItem(makeMenuItem(
            withTitle: "Toggle Tick",
            action: #selector(toggleTick(_:)),
            keyEquivalent: "k",
            target: self
        ))
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

    private func startTickTimer() {
        tickTimer?.invalidate()
        tickTimer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: true) { [weak self] _ in
            self?.shellState.runTick()
        }
        tickTimer?.tolerance = 0.05
    }
}

final class ShellSmokeState: ObservableObject {
    @Published private(set) var lastCommand = "Ready"
    @Published private(set) var resourceStatus: String
    @Published private(set) var saveStatus: String
    @Published private(set) var renderFrame = u3_render_make_synthetic_tile_frame()
    @Published private(set) var tickStatus = "Tick 0 phase 0 input 0 audio 0 running"

    private let inputAdapter = ShellInputAdapter()
    private let audioAdapter = ShellAudioAdapter()
    private let locationProvider = ShellLocationProvider()
    private let resourceAdapter = ShellResourceAdapter()
    private let saveAdapter = ShellSaveAdapter()
    private var tickState = u3_tick_state()
    let coreHeadingProbe: Int8 = u3_map_math_get_heading(1)

    init() {
        u3_tick_state_init(&tickState)
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
        guard inputAdapter.enqueueKeyboard(key) else {
            lastCommand = "Input queue overflow"
            return
        }
        lastCommand = "Queued keyboard \(describeKey(UInt16(key)))"
    }

    func submitMouseDown(x: Int16, y: Int16) {
        guard inputAdapter.enqueueMouseDown(x: x, y: y) else {
            lastCommand = "Input queue overflow"
            return
        }
        lastCommand = "Queued mouse \(x),\(y)"
    }

    func submitMenuCommand(_ command: Int32) {
        guard inputAdapter.enqueueMenuCommand(command) else {
            lastCommand = "Input queue overflow"
            return
        }
        lastCommand = "Queued menu \(command)"
    }

    func submitAudioSound(_ sound: Int32) {
        guard audioAdapter.enqueueSound(sound) else {
            lastCommand = "Audio queue overflow"
            return
        }
        lastCommand = "Queued audio \(sound)"
    }

    func runTick() {
        let inputDescription = inputAdapter.consumeNextDescription()
        let audioDescription = audioAdapter.consumeNextDescription()
        let consumedInput = inputDescription != "Input queue empty"
        let dispatchedAudio = audioDescription != "Audio queue empty"
        var tickInput = u3_tick_input(
            consumed_input: consumedInput ? 1 : 0,
            dispatched_audio: dispatchedAudio ? 1 : 0
        )
        var tickResult = u3_tick_result()

        guard u3_tick_advance(&tickState, &tickInput, &tickResult) != 0 else {
            tickStatus = "Tick failed"
            return
        }

        tickStatus = "Tick \(tickResult.tick_count) phase \(tickResult.phase) input \(tickState.consumed_input_count) audio \(tickState.dispatched_audio_count) running"
        if consumedInput && dispatchedAudio {
            lastCommand = "\(inputDescription) | \(audioDescription)"
        } else if consumedInput {
            lastCommand = inputDescription
        } else if dispatchedAudio {
            lastCommand = audioDescription
        }
    }

    func setTickActive(_ active: Bool) {
        let state = active ? "running" : "paused"
        tickStatus = "Tick \(tickState.tick_count) phase \(tickState.phase) input \(tickState.consumed_input_count) audio \(tickState.dispatched_audio_count) \(state)"
        lastCommand = active ? "Tick resumed" : "Tick paused"
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

    func inspectPartyRoster() {
        let locations = locationProvider.snapshot()
        saveStatus = resourceAdapter.inspectPartyRoster(resourceRootPath: locations.resourceRootPath)
        lastCommand = "Party roster"
    }

    private func describeKey(_ command: UInt16) -> String {
        guard command >= 32 && command <= 126,
              let scalar = UnicodeScalar(Int(command)) else {
            return "#\(command)"
        }

        return String(Character(scalar))
    }

    private static func describeResourceStatus(locations: ShellLocationSnapshot, validation: String, renderStatus: String) -> String {
        guard let resourceRootPath = locations.resourceRootPath else {
            return "\(validation) | \(renderStatus)"
        }

        return "\(validation) | \(renderStatus) | root \(resourceRootPath)"
    }
}
