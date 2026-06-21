import AppKit
import SwiftUI
import Ultima3Core

final class AppDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate {
    private var window: NSWindow?
    private var preferencesWindowController: NSWindowController?
    private var characterCreationWindowController: NSWindowController?
    private var partyAssemblyWindowController: NSWindowController?
    private var suppressCharacterCreationCloseStatus = false
    private var suppressPartyAssemblyCloseStatus = false
    private var discardUnsavedChangesOnTermination = false
    private var tickTimer: Timer?
    private let shellState = ShellSmokeState()

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.mainMenu = makeMainMenu()
        openMainWindow()
        shellState.loadSavedGameIfAvailable()
        startTickTimer()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        true
    }

    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        if discardUnsavedChangesOnTermination {
            discardUnsavedChangesOnTermination = false
            return .terminateNow
        }
        guard shellState.hasUnsavedChanges else {
            return .terminateNow
        }

        let alert = NSAlert()
        alert.messageText = "Save changes before quitting?"
        alert.informativeText = "Unsaved gameplay changes are kept in memory until you save."
        alert.addButton(withTitle: "Save and Quit")
        alert.addButton(withTitle: "Quit Without Saving")
        alert.addButton(withTitle: "Cancel")

        switch alert.runModal() {
        case .alertFirstButtonReturn:
            shellState.saveGame()
            return shellState.hasUnsavedChanges ? .terminateCancel : .terminateNow
        case .alertSecondButtonReturn:
            return .terminateNow
        default:
            return .terminateCancel
        }
    }

    func applicationWillTerminate(_ notification: Notification) {
        tickTimer?.invalidate()
        tickTimer = nil
    }

    @objc private func newGame(_ sender: Any?) {
        guard confirmDiscardUnsavedChanges(for: "starting a new game") else {
            return
        }
        shellState.newGame()
    }

    @objc private func saveGame(_ sender: Any?) {
        shellState.saveGame()
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

    @objc private func loadGame(_ sender: Any?) {
        guard confirmDiscardUnsavedChanges(for: "loading the saved game") else {
            return
        }
        shellState.loadGame()
    }

    private func confirmDiscardUnsavedChanges(for action: String) -> Bool {
        guard shellState.hasUnsavedChanges else {
            return true
        }

        let alert = NSAlert()
        alert.messageText = "Discard unsaved changes?"
        alert.informativeText = "Unsaved gameplay changes will be lost before \(action)."
        alert.addButton(withTitle: "Discard Changes")
        alert.addButton(withTitle: "Cancel")
        return alert.runModal() == .alertFirstButtonReturn
    }

    @objc private func inspectPartyRoster(_ sender: Any?) {
        shellState.inspectPartyRoster()
    }

    @objc private func showCharacterCreation(_ sender: Any?) {
        let hostingController = NSHostingController(rootView: CharacterCreationView(
            onAccept: { [weak self] draft in
                self?.shellState.acceptCharacterCandidate(draft)
                self?.closeCharacterCreationPanel()
            },
            onCancel: { [weak self] in
                self?.shellState.cancelCharacterCandidate()
                self?.closeCharacterCreationPanel()
            }
        ))

        if characterCreationWindowController == nil {
            let panel = NSWindow(contentViewController: hostingController)
            panel.title = "Create Character"
            panel.styleMask = [.titled, .closable]
            panel.setContentSize(NSSize(width: 540, height: 430))
            panel.delegate = self
            characterCreationWindowController = NSWindowController(window: panel)
        } else {
            characterCreationWindowController?.contentViewController = hostingController
        }

        characterCreationWindowController?.showWindow(sender)
        NSApp.activate(ignoringOtherApps: true)
    }

    @objc private func showPartyAssembly(_ sender: Any?) {
        let presentation = shellState.partyAssemblyPresentation()
        let hostingController = NSHostingController(rootView: PartyAssemblyView(
            presentation: presentation,
            onAccept: { [weak self] selectedRosterIDs in
                guard let self else {
                    return
                }
                if presentation.hasActiveParty && !self.confirmReplaceParty() {
                    return
                }
                self.shellState.acceptPartyAssembly(selectedRosterIDs)
                self.closePartyAssemblyPanel()
            },
            onCancel: { [weak self] in
                self?.shellState.cancelPartyAssembly()
                self?.closePartyAssemblyPanel()
            }
        ))

        if partyAssemblyWindowController == nil {
            let panel = NSWindow(contentViewController: hostingController)
            panel.title = "Assemble Party"
            panel.styleMask = [.titled, .closable]
            panel.setContentSize(NSSize(width: 560, height: 400))
            panel.delegate = self
            partyAssemblyWindowController = NSWindowController(window: panel)
        } else {
            partyAssemblyWindowController?.contentViewController = hostingController
        }

        partyAssemblyWindowController?.showWindow(sender)
        NSApp.activate(ignoringOtherApps: true)
    }

    func windowWillClose(_ notification: Notification) {
        guard let window = notification.object as? NSWindow else {
            return
        }

        if window === characterCreationWindowController?.window {
            if !suppressCharacterCreationCloseStatus {
                shellState.cancelCharacterCandidate()
            }
            suppressCharacterCreationCloseStatus = false
            characterCreationWindowController = nil
        } else if window === partyAssemblyWindowController?.window {
            if !suppressPartyAssemblyCloseStatus {
                shellState.cancelPartyAssembly()
            }
            suppressPartyAssemblyCloseStatus = false
            partyAssemblyWindowController = nil
        }
    }

    func windowShouldClose(_ sender: NSWindow) -> Bool {
        guard sender === window else {
            return true
        }
        guard shellState.hasUnsavedChanges else {
            NSApp.terminate(nil)
            return false
        }

        let alert = NSAlert()
        alert.messageText = "Save changes before closing?"
        alert.informativeText = "Unsaved gameplay changes are kept in memory until you save."
        alert.addButton(withTitle: "Save and Close")
        alert.addButton(withTitle: "Close Without Saving")
        alert.addButton(withTitle: "Cancel")

        switch alert.runModal() {
        case .alertFirstButtonReturn:
            shellState.saveGame()
            if !shellState.hasUnsavedChanges {
                NSApp.terminate(nil)
            }
            return false
        case .alertSecondButtonReturn:
            discardUnsavedChangesOnTermination = true
            NSApp.terminate(nil)
            return false
        default:
            return false
        }
    }

    private func closeCharacterCreationPanel() {
        guard let controller = characterCreationWindowController else {
            return
        }

        suppressCharacterCreationCloseStatus = true
        controller.close()
        suppressCharacterCreationCloseStatus = false
        characterCreationWindowController = nil
    }

    private func closePartyAssemblyPanel() {
        guard let controller = partyAssemblyWindowController else {
            return
        }

        suppressPartyAssemblyCloseStatus = true
        controller.close()
        suppressPartyAssemblyCloseStatus = false
        partyAssemblyWindowController = nil
    }

    private func confirmReplaceParty() -> Bool {
        let alert = NSAlert()
        alert.messageText = "Replace Active Party?"
        alert.informativeText = "The selected roster members will replace the current active party."
        alert.addButton(withTitle: "Replace")
        alert.addButton(withTitle: "Cancel")
        return alert.runModal() == .alertFirstButtonReturn
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
        window.delegate = self
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
            withTitle: "Load Game",
            action: #selector(loadGame(_:)),
            keyEquivalent: "o",
            target: self
        ))
        gameMenu.addItem(NSMenuItem.separator())
        gameMenu.addItem(makeMenuItem(
            withTitle: "Refresh Locations",
            action: #selector(refreshLocations(_:)),
            keyEquivalent: "l",
            target: self
        ))
        gameMenu.addItem(makeMenuItem(
            withTitle: "Inspect Party/Roster",
            action: #selector(inspectPartyRoster(_:)),
            keyEquivalent: "i",
            target: self
        ))
        gameMenu.addItem(makeMenuItem(
            withTitle: "Create Character...",
            action: #selector(showCharacterCreation(_:)),
            keyEquivalent: "c",
            target: self
        ))
        gameMenu.addItem(makeMenuItem(
            withTitle: "Assemble Party...",
            action: #selector(showPartyAssembly(_:)),
            keyEquivalent: "p",
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

    private let inputAdapter: ShellInputAdapter
    private let audioAdapter: ShellAudioAdapter
    private let locationProvider: ShellLocationProvider
    private let resourceAdapter: ShellResourceAdapter
    private let saveAdapter: ShellSaveAdapter
    private let characterCreationAdapter = ShellCharacterCreationAdapter()
    private let partyAssemblyAdapter = ShellPartyAssemblyAdapter()
    private var tickState = u3_tick_state()
    private var overworldState = u3_overworld_state()
    private var overworldMapData: Data?
    private var currentSaveDocument: Data?
    @Published private(set) var hasUnsavedChanges = false
    let coreHeadingProbe: Int8 = u3_map_math_get_heading(1)

    init(
        inputAdapter: ShellInputAdapter = ShellInputAdapter(),
        audioAdapter: ShellAudioAdapter = ShellAudioAdapter(),
        locationProvider: ShellLocationProvider = ShellLocationProvider(),
        resourceAdapter: ShellResourceAdapter = ShellResourceAdapter(),
        saveAdapter: ShellSaveAdapter = ShellSaveAdapter()
    ) {
        self.inputAdapter = inputAdapter
        self.audioAdapter = audioAdapter
        self.locationProvider = locationProvider
        self.resourceAdapter = resourceAdapter
        self.saveAdapter = saveAdapter
        u3_tick_state_init(&tickState)
        u3_overworld_state_init(
            &overworldState,
            5,
            5,
            UInt8(U3_OVERWORLD_SMOKE_VIEW_WIDTH),
            UInt8(U3_OVERWORLD_SMOKE_VIEW_HEIGHT)
        )
        let locations = locationProvider.snapshot()
        let overworldSmoke = resourceAdapter.buildOverworldSmoke(resourceRootPath: locations.resourceRootPath, state: &overworldState)
        overworldMapData = overworldSmoke.map
        renderFrame = overworldSmoke.frame
        resourceStatus = Self.describeResourceStatus(
            locations: locations,
            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
            renderStatus: overworldSmoke.status
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

    func submitOverworldCommand(_ command: UInt16) {
        guard inputAdapter.enqueueKeyboardCommand(command) else {
            lastCommand = "Input queue overflow"
            return
        }
        lastCommand = "Queued move \(inputAdapter.describeKey(command))"
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
        let inputEvent = inputAdapter.consumeNextEvent()
        let inputDescription = inputEvent.map { consumeInputEvent($0) } ?? "Input queue empty"
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
        let overworldSmoke = resourceAdapter.buildOverworldSmoke(resourceRootPath: locations.resourceRootPath, state: &overworldState)
        overworldMapData = overworldSmoke.map
        renderFrame = overworldSmoke.frame
        resourceStatus = Self.describeResourceStatus(
            locations: locations,
            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
            renderStatus: overworldSmoke.status
        )
        saveStatus = locations.saveStatus
        lastCommand = "Locations refreshed"
    }

    func saveGame() {
        let locations = locationProvider.snapshot()
        guard let document = currentSaveDocument else {
            saveStatus = "Save unavailable: start or load a game first"
            lastCommand = "Save unavailable"
            return
        }
        guard resourceAdapter.isValidSaveDomain(document) else {
            saveStatus = "Save rejected: invalid current game"
            lastCommand = "Save rejected"
            return
        }
        guard resourceAdapter.currentSosariaSaveAllowed(documentData: document) else {
            saveStatus = "Save unavailable outside Sosaria"
            lastCommand = "Save rejected"
            return
        }

        let result = saveAdapter.writeDocument(document, saveDocumentPath: locations.saveDocumentPath)
        saveStatus = "\(result.message) | \(locations.saveDocumentPath)"
        if result == .saved {
            hasUnsavedChanges = false
            lastCommand = "Game saved"
        } else {
            lastCommand = "Save failed"
        }
    }

    func newGame() {
        let locations = locationProvider.snapshot()
        guard let document = resourceAdapter.buildNativeNewGameSmokeDocument(resourceRootPath: locations.resourceRootPath) else {
            saveStatus = "New game failed: could not build save document"
            lastCommand = "New game failed"
            return
        }

        applyCurrentSaveDocument(document, locations: locations, statusPrefix: "Current Save")
        hasUnsavedChanges = true
        lastCommand = "New game started"
    }

    func loadGame() {
        let locations = locationProvider.snapshot()
        guard let document = saveAdapter.readDocument(saveDocumentPath: locations.saveDocumentPath) else {
            saveStatus = "No saved game found | \(locations.saveDocumentPath)"
            lastCommand = "Load failed"
            return
        }
        guard resourceAdapter.isValidSaveDomain(document) else {
            saveStatus = resourceAdapter.describeSaveDomain(document, prefix: "Saved Game")
            lastCommand = "Load failed"
            return
        }

        applyCurrentSaveDocument(document, locations: locations, statusPrefix: "Saved Game")
        hasUnsavedChanges = false
        lastCommand = "Game loaded"
    }

    func loadSavedGameIfAvailable() {
        let locations = locationProvider.snapshot()
        guard let document = saveAdapter.readDocument(saveDocumentPath: locations.saveDocumentPath),
              resourceAdapter.isValidSaveDomain(document) else {
            return
        }

        applyCurrentSaveDocument(document, locations: locations, statusPrefix: "Saved Game")
        hasUnsavedChanges = false
        lastCommand = "Game loaded"
    }

    private func consumeInputEvent(_ event: u3_input_event) -> String {
        if Int32(event.kind) == U3_INPUT_EVENT_KEYBOARD {
            var moveResult = u3_overworld_move_result()
            if consumeOverworldMovement(event.command, result: &moveResult),
               moveResult.handled != 0 {
                persistOverworldTurnToCurrentSave(result: &moveResult)
                if moveResult.redraw != 0 {
                    renderOverworldFrame()
                }
                if moveResult.sound_id != 0 {
                    _ = audioAdapter.enqueueSound(Int32(moveResult.sound_id))
                }

                switch Int32(moveResult.status) {
                case U3_OVERWORLD_STATUS_BLOCKED:
                    return "Blocked \(inputAdapter.describeKey(event.command)) \(moveResult.x),\(moveResult.y) tile \(moveResult.target_tile)\(describeTurnDelta(moveResult))"
                case U3_OVERWORLD_STATUS_MOVED:
                    return "Move \(inputAdapter.describeKey(event.command)) \(moveResult.x),\(moveResult.y)\(describeTurnDelta(moveResult))"
                default:
                    return inputAdapter.describe(event)
                }
            }

            var transitionResult = u3_location_transition_result()
            if resourceAdapter.handleOverworldLocationCommand(
                documentData: currentSaveDocument,
                mapData: overworldMapData,
                state: &overworldState,
                command: event.command,
                result: &transitionResult
            ), transitionResult.handled != 0 {
                switch Int32(transitionResult.status) {
                case U3_LOCATION_STATUS_TOWN_REQUESTED:
                    return "Enter town index \(transitionResult.location_index) MAPS \(transitionResult.resource_id) return \(transitionResult.return_x),\(transitionResult.return_y) start \(transitionResult.initial_x),\(transitionResult.initial_y) heading \(transitionResult.initial_heading)"
                case U3_LOCATION_STATUS_NOT_ENTERABLE:
                    return "Enter unavailable \(transitionResult.return_x),\(transitionResult.return_y)"
                default:
                    return inputAdapter.describe(event)
                }
            }
        }

        return inputAdapter.describe(event)
    }

    private func consumeOverworldMovement(_ command: UInt16, result: inout u3_overworld_move_result) -> Bool {
        guard let overworldMapData else {
            return u3_overworld_move(&overworldState, command, &result) != 0
        }

        return overworldMapData.withUnsafeBytes { mapBuffer in
            guard let mapBaseAddress = mapBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }

            return u3_overworld_move_on_map(&overworldState, mapBaseAddress, UInt32(overworldMapData.count), command, &result) != 0
        }
    }

    private func renderOverworldFrame() {
        guard let overworldMapData else {
            renderFrame = u3_render_make_synthetic_tile_frame()
            return
        }

        renderFrame = overworldMapData.withUnsafeBytes { mapBuffer in
            guard let mapBaseAddress = mapBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return u3_render_make_synthetic_tile_frame()
            }

            return u3_overworld_make_view_frame(mapBaseAddress, UInt32(overworldMapData.count), &overworldState)
        }
        let locations = locationProvider.snapshot()
        resourceStatus = Self.describeResourceStatus(
            locations: locations,
            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
            renderStatus: "Overworld OK MAPS 419 pos \(overworldState.x),\(overworldState.y)"
        )
    }

    private func persistOverworldTurnToCurrentSave(result: inout u3_overworld_move_result) {
        guard var documentData = currentSaveDocument else {
            return
        }

        let documentLength = documentData.count
        let updated = documentData.withUnsafeMutableBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }

            var document = u3_save_document()
            guard u3_save_open(documentBaseAddress, documentLength, &document) != 0 else {
                return false
            }

            var partyRecord = u3_save_record()
            guard u3_save_find_record(&document, Self.fourCharacterCode("PRTY"), Int16(U3_SAVE_ID_PARTY), &partyRecord) != 0,
                  partyRecord.length == U3_SAVE_PARTY_LENGTH,
                  let party = UnsafeMutableRawPointer(mutating: partyRecord.data)?.assumingMemoryBound(to: UInt8.self) else {
                return false
            }

            if result.moved != 0,
               u3_overworld_write_party_position(party, partyRecord.length, &overworldState) == 0 {
                return false
            }

            return u3_overworld_increment_party_move_counter(party, partyRecord.length, &result) != 0
        }

        if updated {
            currentSaveDocument = documentData
            hasUnsavedChanges = true
        }
    }

    private func applyCurrentSaveDocument(_ document: Data, locations: ShellLocationSnapshot, statusPrefix: String) {
        currentSaveDocument = document
        saveStatus = resourceAdapter.describeSaveDomain(document, prefix: statusPrefix)
        let overworldSmoke = resourceAdapter.buildOverworldSmoke(documentData: document, state: &overworldState)
        overworldMapData = overworldSmoke.map
        renderFrame = overworldSmoke.frame
        resourceStatus = Self.describeResourceStatus(
            locations: locations,
            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
            renderStatus: overworldSmoke.status
        )
    }

    func inspectPartyRoster() {
        if let currentSaveDocument {
            saveStatus = resourceAdapter.inspectPartyRoster(documentData: currentSaveDocument)
        } else {
            let locations = locationProvider.snapshot()
            saveStatus = resourceAdapter.inspectPartyRoster(resourceRootPath: locations.resourceRootPath)
        }
        lastCommand = "Party roster"
    }

    func acceptCharacterCandidate(_ draft: ShellCharacterDraft) {
        let result = characterCreationAdapter.persist(draft, currentDocument: currentSaveDocument)
        guard result.saved, let document = result.document else {
            saveStatus = result.message
            lastCommand = "Character save failed"
            return
        }

        currentSaveDocument = document
        hasUnsavedChanges = true
        saveStatus = resourceAdapter.inspectPartyRoster(documentData: document)
        lastCommand = "Character saved roster \(result.rosterID)"
    }

    func cancelCharacterCandidate() {
        lastCommand = "Character creation cancelled"
    }

    func partyAssemblyPresentation() -> ShellPartyAssemblyPresentation {
        partyAssemblyAdapter.presentation(currentDocument: currentSaveDocument)
    }

    func acceptPartyAssembly(_ selectedRosterIDs: [UInt8]) {
        let result = partyAssemblyAdapter.assemble(selectedRosterIDs: selectedRosterIDs, currentDocument: currentSaveDocument)
        guard result.assembled, let document = result.document else {
            saveStatus = result.message
            lastCommand = "Party assembly failed"
            return
        }

        currentSaveDocument = document
        hasUnsavedChanges = true
        saveStatus = resourceAdapter.inspectPartyRoster(documentData: document)
        lastCommand = result.message
    }

    func cancelPartyAssembly() {
        lastCommand = "Party assembly cancelled"
    }

    private func describeKey(_ command: UInt16) -> String {
        guard command >= 32 && command <= 126,
              let scalar = UnicodeScalar(Int(command)) else {
            return "#\(command)"
        }

        return String(Character(scalar))
    }

    private func describeTurnDelta(_ result: u3_overworld_move_result) -> String {
        guard result.turn_applied != 0 else {
            return ""
        }

        return " moves \(result.move_counter_after)"
    }

    private static func describeResourceStatus(locations: ShellLocationSnapshot, validation: String, renderStatus: String) -> String {
        guard let resourceRootPath = locations.resourceRootPath else {
            return "\(validation) | \(renderStatus)"
        }

        return "\(validation) | \(renderStatus) | root \(resourceRootPath)"
    }

    private static func fourCharacterCode(_ value: String) -> UInt32 {
        var result: UInt32 = 0

        for byte in value.utf8.prefix(4) {
            result = (result << 8) | UInt32(byte)
        }

        return result
    }
}
