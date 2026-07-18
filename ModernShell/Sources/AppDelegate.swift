import AppKit
import SwiftUI
import Ultima3Core

enum ShellTerminationAction {
    case saveAndQuit
    case quitWithoutSaving
    case cancel
}

enum ShellTerminationDecision: Equatable {
    case terminate
    case cancel
}

struct ShellTerminationPolicy {
    static func decision(
        action: ShellTerminationAction,
        hasUnsavedChangesAfterSave: Bool
    ) -> ShellTerminationDecision {
        switch action {
        case .saveAndQuit:
            return hasUnsavedChangesAfterSave ? .cancel : .terminate
        case .quitWithoutSaving:
            return .terminate
        case .cancel:
            return .cancel
        }
    }
}

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
            return ShellTerminationPolicy.decision(
                action: .saveAndQuit,
                hasUnsavedChangesAfterSave: shellState.hasUnsavedChanges
            ) == .terminate ? .terminateNow : .terminateCancel
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
            if ShellTerminationPolicy.decision(
                action: .saveAndQuit,
                hasUnsavedChangesAfterSave: shellState.hasUnsavedChanges
            ) == .terminate {
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

protocol ShellLocationDocumentTransitioning {
    func applyEntry(
        to documentData: inout Data,
        request: inout u3_location_transition_result
    ) -> Bool
    func applyMove(
        to documentData: inout Data,
        session: ShellLocationSession,
        result: inout u3_location_move_result
    ) -> Bool
}

final class ShellLocationDocumentTransitionAdapter: ShellLocationDocumentTransitioning {
    func applyEntry(
        to documentData: inout Data,
        request: inout u3_location_transition_result
    ) -> Bool {
        withMutableParty(in: &documentData) { party, length in
            u3_location_enter_party(party, length, &request) != 0
        }
    }

    func applyMove(
        to documentData: inout Data,
        session: ShellLocationSession,
        result: inout u3_location_move_result
    ) -> Bool {
        withMutableParty(in: &documentData) { party, length in
            guard u3_location_apply_party_turn(party, length, &result) != 0 else {
                return false
            }
            if result.exit_requested != 0 {
                var descriptor = session.descriptor
                return u3_location_restore_party(party, length, &descriptor) != 0
            }
            return true
        }
    }

    private func withMutableParty(
        in documentData: inout Data,
        mutation: (UnsafeMutablePointer<UInt8>, UInt32) -> Bool
    ) -> Bool {
        let documentLength = documentData.count
        return documentData.withUnsafeMutableBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return false
            }

            var document = u3_save_document()
            guard u3_save_open(documentBaseAddress, documentLength, &document) != 0 else {
                return false
            }

            var partyRecord = u3_save_record()
            guard u3_save_find_record(&document, 0x50525459, Int16(U3_SAVE_ID_PARTY), &partyRecord) != 0,
                  partyRecord.length == U3_SAVE_PARTY_LENGTH,
                  let party = UnsafeMutableRawPointer(mutating: partyRecord.data)?.assumingMemoryBound(to: UInt8.self) else {
                return false
            }

            return mutation(party, partyRecord.length)
        }
    }
}

final class ShellSmokeState: ObservableObject {
    typealias CombatMonsterRolls = (
        shootChoice: UInt8,
        magicChoice: UInt8,
        magicTarget: UInt8,
        projectileHit: UInt8,
        poison: UInt8,
        pilferBranch: UInt8,
        pilferItem: UInt8,
        armourHit: UInt8,
        damage: UInt8,
        exodusDamage: UInt8
    )
    typealias CombatPlayerRolls = (
        hitChance: UInt8,
        hitDexterity: UInt8,
        damage: UInt8
    )

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
    private let locationTransitionAdapter: ShellLocationDocumentTransitioning
    private let dungeonRollProvider: (UInt8) -> (encounter: UInt16, monster: UInt16)
    private let dungeonChestRollProvider: () -> (trap: UInt16, gold: UInt16)
    private let combatPlayerRollProvider: (UInt8) -> CombatPlayerRolls
    private let combatMonsterRollProvider: () -> CombatMonsterRolls
    private let combatSessionLoader: (String?, u3_dungeon_post_turn_result, ShellLocationSession, Data?) -> ShellCombatSessionLoadResult
    private let characterCreationAdapter = ShellCharacterCreationAdapter()
    private let partyAssemblyAdapter = ShellPartyAssemblyAdapter()
    private var tickState = u3_tick_state()
    private var overworldState = u3_overworld_state()
    private var overworldMapData: Data?
    private var activeLocationSession: ShellLocationSession?
    private var activeCombatSession: ShellCombatSession?
    private var awaitingTalkDirection = false
    private var pendingCombatAttackCharacter: UInt8?
    private var pendingDungeonInteractionCommand: UInt16?
    private var currentSaveDocument: Data?
    @Published private(set) var hasUnsavedChanges = false
    let coreHeadingProbe: Int8 = u3_map_math_get_heading(1)

    init(
        inputAdapter: ShellInputAdapter = ShellInputAdapter(),
        audioAdapter: ShellAudioAdapter = ShellAudioAdapter(),
        locationProvider: ShellLocationProvider = ShellLocationProvider(),
        resourceAdapter: ShellResourceAdapter = ShellResourceAdapter(),
        saveAdapter: ShellSaveAdapter = ShellSaveAdapter(),
        locationTransitionAdapter: ShellLocationDocumentTransitioning = ShellLocationDocumentTransitionAdapter(),
        dungeonRollProvider: @escaping (UInt8) -> (encounter: UInt16, monster: UInt16) = ShellSmokeState.makeDungeonRolls,
        dungeonChestRollProvider: @escaping () -> (trap: UInt16, gold: UInt16) = ShellSmokeState.makeDungeonChestRolls,
        combatPlayerRollProvider: @escaping (UInt8) -> CombatPlayerRolls = ShellSmokeState.makeCombatPlayerRolls,
        combatMonsterRollProvider: @escaping () -> CombatMonsterRolls = ShellSmokeState.makeCombatMonsterRolls,
        combatSessionLoader: ((String?, u3_dungeon_post_turn_result, ShellLocationSession, Data?) -> ShellCombatSessionLoadResult)? = nil
    ) {
        self.inputAdapter = inputAdapter
        self.audioAdapter = audioAdapter
        self.locationProvider = locationProvider
        self.resourceAdapter = resourceAdapter
        self.saveAdapter = saveAdapter
        self.locationTransitionAdapter = locationTransitionAdapter
        self.dungeonRollProvider = dungeonRollProvider
        self.dungeonChestRollProvider = dungeonChestRollProvider
        self.combatPlayerRollProvider = combatPlayerRollProvider
        self.combatMonsterRollProvider = combatMonsterRollProvider
        self.combatSessionLoader = combatSessionLoader ?? { resourceRootPath, encounter, sourceSession, documentData in
            resourceAdapter.loadCombatSession(
                resourceRootPath: resourceRootPath,
                encounter: encounter,
                sourceSession: sourceSession,
                documentData: documentData
            )
        }
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
        awaitingTalkDirection = false
        pendingCombatAttackCharacter = nil
        pendingDungeonInteractionCommand = nil
        if let activeCombatSession {
            renderFrame = activeCombatSession.frame
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: activeCombatSession.status
            )
            lastCommand = "Combat refreshed"
            return
        }
        if let activeLocationSession {
            renderFrame = activeLocationSession.frame
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: activeLocationSession.status
            )
            lastCommand = "Location refreshed"
            return
        }

        let overworldSmoke: ShellOverworldSmokeResult
        if let currentSaveDocument {
            overworldSmoke = resourceAdapter.buildOverworldSmoke(documentData: currentSaveDocument, state: &overworldState)
        } else {
            overworldSmoke = resourceAdapter.buildOverworldSmoke(resourceRootPath: locations.resourceRootPath, state: &overworldState)
        }
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
            if var combatSession = activeCombatSession {
                return consumeCombatInput(event, session: &combatSession)
            }

            if var locationSession = activeLocationSession {
                if locationSession.descriptor.destination_kind == U3_LOCATION_KIND_DUNGEON {
                    return consumeDungeonInput(event, session: &locationSession)
                }

                if Self.normalizedKeyboardCommand(event.command) == UInt16(UInt8(ascii: "I")) {
                    return "Ignite unavailable: not in dungeon"
                }

                if awaitingTalkDirection &&
                    locationSession.descriptor.destination_kind == U3_LOCATION_KIND_TOWN {
                    awaitingTalkDirection = false
                    var talkResult = u3_location_talk_result()
                    guard resourceAdapter.talkLocationSession(
                        locationSession,
                        direction: event.command,
                        result: &talkResult
                    ) else {
                        return "Talk unavailable"
                    }

                    return resourceAdapter.describeLocationTalk(&talkResult)
                }

                if event.command == U3_LOCATION_TALK_COMMAND &&
                    locationSession.descriptor.destination_kind == U3_LOCATION_KIND_TOWN {
                    awaitingTalkDirection = true
                    return "Talk: choose direction"
                }

                var locationResult = u3_location_move_result()
                if resourceAdapter.moveLocationSession(
                    &locationSession,
                    command: event.command,
                    result: &locationResult
                ), locationResult.handled != 0 {
                    guard applyLocationMoveTransaction(
                        session: locationSession,
                        result: &locationResult
                    ) else {
                        return "Location transition failed"
                    }

                    if locationResult.exit_requested != 0 {
                        if locationResult.sound_id != 0 {
                            _ = audioAdapter.enqueueSound(Int32(locationResult.sound_id))
                        }
                        return "Returned to Sosaria \(overworldState.x),\(overworldState.y)\(describeLocationTurnDelta(locationResult))"
                    }

                    activeLocationSession = locationSession
                    if locationResult.redraw != 0 {
                        renderFrame = locationSession.frame
                        let locations = locationProvider.snapshot()
                        resourceStatus = Self.describeResourceStatus(
                            locations: locations,
                            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                            renderStatus: locationSession.status
                        )
                    }
                    if locationResult.sound_id != 0 {
                        _ = audioAdapter.enqueueSound(Int32(locationResult.sound_id))
                    }

                    switch Int32(locationResult.status) {
                    case U3_LOCATION_MOVE_STATUS_BLOCKED:
                        return "Location blocked \(inputAdapter.describeKey(event.command)) \(locationResult.x),\(locationResult.y) tile \(locationResult.target_tile)\(describeLocationTurnDelta(locationResult))"
                    case U3_LOCATION_MOVE_STATUS_MOVED:
                        return "Location move \(inputAdapter.describeKey(event.command)) \(locationResult.x),\(locationResult.y)\(describeLocationTurnDelta(locationResult))"
                    default:
                        return inputAdapter.describe(event)
                    }
                }
                return "Location MAPS \(locationSession.descriptor.resource_id) command \(inputAdapter.describeKey(event.command)) deferred"
            }

            if Self.normalizedKeyboardCommand(event.command) == UInt16(UInt8(ascii: "I")) {
                return "Ignite unavailable: not in dungeon"
            }

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
                    return activateLocationSession(request: transitionResult)
                case U3_LOCATION_STATUS_DUNGEON_REQUESTED:
                    return activateLocationSession(request: transitionResult)
                case U3_LOCATION_STATUS_NOT_ENTERABLE:
                    return "Enter unavailable \(transitionResult.return_x),\(transitionResult.return_y)"
                default:
                    return inputAdapter.describe(event)
                }
            }
        }

        return inputAdapter.describe(event)
    }

    private func consumeCombatInput(_ event: u3_input_event, session: inout ShellCombatSession) -> String {
        if pendingCombatAttackCharacter == nil,
           let activeCharacter = playableCombatCharacter(in: session, startingAt: session.activeCharacter) {
            session.activeCharacter = activeCharacter
        } else if pendingCombatAttackCharacter == nil {
            activeCombatSession = session
            return "Combat has no active characters"
        }

        var input = makeCombatCommandInput(command: Self.normalizedKeyboardCommand(event.command), session: session)

        if let pendingCharacter = pendingCombatAttackCharacter {
            guard let direction = Self.combatDirection(for: event.command) else {
                pendingCombatAttackCharacter = nil
                activeCombatSession = session
                return "Combat attack cancelled"
            }
            input.command = UInt16(U3_COMBAT_COMMAND_ATTACK)
            input.character = pendingCharacter
            input.attack_direction_x = direction.dx
            input.attack_direction_y = direction.dy
        }

        let experience = combatExperienceTable()
        let result = experience.withUnsafeBufferPointer { buffer in
            u3_combat_player_command(&session.combatState, buffer.baseAddress, &input)
        }

        guard result.handled != 0 else {
            activeCombatSession = session
            return "Combat CONS \(session.screenResourceID) command \(inputAdapter.describeKey(event.command)) deferred"
        }

        if result.attack_direction_required != 0 {
            pendingCombatAttackCharacter = input.character
            activeCombatSession = session
            return "Combat character \(input.character + 1) attack: choose direction"
        }

        pendingCombatAttackCharacter = nil
        if result.redraw != 0 {
            resourceAdapter.refreshCombatSessionFrame(&session)
            renderFrame = session.frame
            let locations = locationProvider.snapshot()
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: session.status
            )
        }
        if result.sound_id != 0 {
            _ = audioAdapter.enqueueSound(Int32(result.sound_id))
        }

        var description = describeCombatPlayerCommand(result, session: session)
        var damageResult = result.attack_result.damage_result
        let experienceAward = u3_combat_apply_experience_award(&session.combatState, &damageResult)
        if experienceAward.applied != 0,
           var documentData = currentSaveDocument,
           resourceAdapter.applyCombatPartyState(session, documentData: &documentData) {
            currentSaveDocument = documentData
            hasUnsavedChanges = true
            description += " exp \(experienceAward.experience_after)"
        }
        let victoryResult = u3_combat_check_victory(&session.combatState)
        if victoryResult.victorious != 0 {
            return restoreAfterCombatVictory(session: session, description: description, victory: victoryResult)
        }
        if result.status == UInt8(U3_COMBAT_PLAYER_STATUS_MOVED) ||
            result.status == UInt8(U3_COMBAT_PLAYER_STATUS_PASSED) ||
            result.status == UInt8(U3_COMBAT_PLAYER_STATUS_ATTACK_HIT) ||
            result.status == UInt8(U3_COMBAT_PLAYER_STATUS_ATTACK_MISSED) {
            advanceCombatCharacter(&session)
            description += runCombatMonsterTurn(session: &session)
        }
        activeCombatSession = session
        return description
    }

    private func combatExperienceTable() -> [UInt8] {
        guard let currentSaveDocument else {
            return [UInt8](repeating: 0, count: Int(U3_COMBAT_EXPERIENCE_COUNT))
        }

        return currentSaveDocument.withUnsafeBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return [UInt8](repeating: 0, count: Int(U3_COMBAT_EXPERIENCE_COUNT))
            }

            var document = u3_save_document()
            var experienceRecord = u3_save_record()
            guard u3_save_open(documentBaseAddress, currentSaveDocument.count, &document) != 0,
                  u3_save_find_record(&document, Self.fourCharacterCode("MISC"), Int16(U3_SAVE_ID_MISC_BASE + 5), &experienceRecord) != 0,
                  let experience = experienceRecord.data else {
                return [UInt8](repeating: 0, count: Int(U3_COMBAT_EXPERIENCE_COUNT))
            }

            var table = [UInt8](repeating: 0, count: Int(U3_COMBAT_EXPERIENCE_COUNT))
            let copied = min(table.count, Int(experienceRecord.length))
            for index in 0..<copied {
                table[index] = experience[index]
            }
            return table
        }
    }

    private func restoreAfterCombatVictory(
        session: ShellCombatSession,
        description: String,
        victory: u3_combat_victory_result
    ) -> String {
        var sourceSession = session.sourceLocationSession
        guard sourceSession.descriptor.active != 0,
              sourceSession.descriptor.destination_kind == U3_LOCATION_KIND_DUNGEON else {
            activeCombatSession = session
            return "\(description) | Combat victory restore failed"
        }

        resourceAdapter.refreshLocationSessionFrame(&sourceSession)
        activeCombatSession = nil
        activeLocationSession = sourceSession
        pendingCombatAttackCharacter = nil
        pendingDungeonInteractionCommand = nil
        awaitingTalkDirection = false
        renderFrame = sourceSession.frame
        let locations = locationProvider.snapshot()
        resourceStatus = Self.describeResourceStatus(
            locations: locations,
            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
            renderStatus: sourceSession.status
        )
        return "\(description) | Combat victory remaining \(victory.live_monsters) returned dungeon MAPS \(sourceSession.descriptor.resource_id)"
    }

    private func makeCombatCommandInput(command: UInt16, session: ShellCombatSession) -> u3_combat_player_command_input {
        let character = session.activeCharacter
        let roster = combatRosterSnapshot(character: character, session: session)
        let rolls = combatPlayerRollProvider(roster.strength)
        return u3_combat_player_command_input(
            command: command,
            character: character,
            attack_direction_x: 0,
            attack_direction_y: 0,
            weapon: roster.weapon,
            weapon_quantity: roster.weaponQuantity,
            strength: roster.strength,
            dexterity: roster.dexterity,
            projectile_monster: UInt8(U3_COMBAT_NO_SLOT),
            exodus_castle_result: 0xFF,
            hit_chance_roll: rolls.hitChance,
            hit_dexterity_roll: rolls.hitDexterity,
            damage_roll: rolls.damage
        )
    }

    private func combatRosterSnapshot(
        character: UInt8,
        session: ShellCombatSession
    ) -> (strength: UInt8, dexterity: UInt8, weapon: UInt8, weaponQuantity: UInt8) {
        let rosterID = combatRosterID(character: character, session: session)
        guard rosterID > 0,
              let currentSaveDocument else {
            return (10, 10, 0, 0)
        }

        return currentSaveDocument.withUnsafeBytes { documentBuffer in
            guard let documentBaseAddress = documentBuffer.bindMemory(to: UInt8.self).baseAddress else {
                return (10, 10, 0, 0)
            }

            var document = u3_save_document()
            var rosterRecord = u3_save_record()
            guard u3_save_open(documentBaseAddress, currentSaveDocument.count, &document) != 0,
                  u3_save_find_record(&document, Self.fourCharacterCode("ROST"), Int16(U3_SAVE_ID_ROSTER), &rosterRecord) != 0,
                  rosterRecord.length == U3_SAVE_ROSTER_LENGTH,
                  let roster = rosterRecord.data else {
                return (10, 10, 0, 0)
            }

            let record = roster + ((Int(rosterID) - 1) * Int(U3_PARTY_ROSTER_RECORD_LENGTH))
            return (
                record[18],
                record[19],
                record[48],
                record[49]
            )
        }
    }

    private func combatRosterID(character: UInt8, session: ShellCombatSession) -> UInt8 {
        switch character {
        case 0:
            return session.activeRosterIDs.0
        case 1:
            return session.activeRosterIDs.1
        case 2:
            return session.activeRosterIDs.2
        case 3:
            return session.activeRosterIDs.3
        default:
            return 0
        }
    }

    private func advanceCombatCharacter(_ session: inout ShellCombatSession) {
        guard session.partySize > 0 else {
            session.activeCharacter = 0
            return
        }
        let cappedPartySize = min(session.partySize, UInt8(U3_COMBAT_CHARACTER_COUNT))
        let nextCharacter = (session.activeCharacter + 1) % cappedPartySize
        session.activeCharacter = playableCombatCharacter(in: session, startingAt: nextCharacter) ?? session.activeCharacter
    }

    private func runCombatMonsterTurn(session: inout ShellCombatSession) -> String {
        var input = makeCombatMonsterTurnInput(session: session)
        let result = u3_combat_monster_turn(&session.combatState, &input)
        session.nextMonster = result.next_starting_monster

        if result.redraw != 0 {
            resourceAdapter.refreshCombatSessionFrame(&session)
            renderFrame = session.frame
            let locations = locationProvider.snapshot()
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: session.status
            )
        }
        if result.sound_id != 0 {
            _ = audioAdapter.enqueueSound(Int32(result.sound_id))
        }
        if combatMonsterTurnMutatedParty(result),
           var documentData = currentSaveDocument,
           resourceAdapter.applyCombatPartyState(session, documentData: &documentData) {
            currentSaveDocument = documentData
            hasUnsavedChanges = true
        }

        return " | \(describeCombatMonsterTurn(result))"
    }

    private func makeCombatMonsterTurnInput(session: ShellCombatSession) -> u3_combat_monster_turn_input {
        let rolls = combatMonsterRollProvider()
        let cappedPartySize = max(UInt8(1), min(session.partySize, UInt8(U3_COMBAT_CHARACTER_COUNT)))
        return u3_combat_monster_turn_input(
            party_size: session.partySize,
            starting_monster: session.nextMonster,
            shoot_choice_roll: rolls.shootChoice,
            magic_choice_roll: rolls.magicChoice,
            magic_target_character: rolls.magicTarget % cappedPartySize,
            projectile_hit_character: rolls.projectileHit % cappedPartySize,
            poison_roll: rolls.poison,
            pilfer_branch_roll: rolls.pilferBranch,
            pilfer_item_roll: rolls.pilferItem,
            exodus_castle_active: 0,
            exodus_damage_flags: rolls.exodusDamage,
            armour_hit_roll: rolls.armourHit,
            damage_roll: rolls.damage
        )
    }

    private func combatMonsterTurnMutatedParty(_ result: u3_combat_monster_turn_result) -> Bool {
        result.action_result.hit != 0 ||
            result.action_result.poisoned != 0 ||
            result.action_result.pilfered != 0 ||
            result.action_result.character_died != 0
    }

    private func playableCombatCharacter(in session: ShellCombatSession, startingAt start: UInt8) -> UInt8? {
        let cappedPartySize = min(session.partySize, UInt8(U3_COMBAT_CHARACTER_COUNT))
        guard cappedPartySize > 0 else {
            return nil
        }

        for offset in 0..<cappedPartySize {
            let candidate = (start + offset) % cappedPartySize
            if combatCharacterCanAct(candidate, in: session) {
                return candidate
            }
        }
        return nil
    }

    private func combatCharacterCanAct(_ character: UInt8, in session: ShellCombatSession) -> Bool {
        let status: UInt8
        let x: UInt8
        let y: UInt8
        switch character {
        case 0:
            status = session.combatState.character_status.0
            x = session.combatState.character_x.0
            y = session.combatState.character_y.0
        case 1:
            status = session.combatState.character_status.1
            x = session.combatState.character_x.1
            y = session.combatState.character_y.1
        case 2:
            status = session.combatState.character_status.2
            x = session.combatState.character_x.2
            y = session.combatState.character_y.2
        case 3:
            status = session.combatState.character_status.3
            x = session.combatState.character_x.3
            y = session.combatState.character_y.3
        default:
            return false
        }

        if status == UInt8(ascii: "D") || status == UInt8(ascii: "A") {
            return false
        }
        return x <= 10 && y <= 10
    }

    private func describeCombatPlayerCommand(
        _ result: u3_combat_player_command_result,
        session: ShellCombatSession
    ) -> String {
        let character = result.character + 1
        switch Int32(result.status) {
        case U3_COMBAT_PLAYER_STATUS_MOVED:
            return "Combat character \(character) move \(result.x),\(result.y)"
        case U3_COMBAT_PLAYER_STATUS_BLOCKED:
            return "Combat character \(character) blocked \(result.target_x),\(result.target_y) tile \(result.target_tile)"
        case U3_COMBAT_PLAYER_STATUS_PASSED:
            return "Combat character \(character) pass"
        case U3_COMBAT_PLAYER_STATUS_ATTACK_HIT:
            if result.attack_result.damage_result.defeated != 0 {
                return "Combat character \(character) hit monster \(result.attack_result.target_monster) defeated damage \(result.attack_result.damage_amount)"
            }
            return "Combat character \(character) hit monster \(result.attack_result.target_monster) damage \(result.attack_result.damage_amount)"
        case U3_COMBAT_PLAYER_STATUS_ATTACK_MISSED:
            return "Combat character \(character) attack missed"
        case U3_COMBAT_PLAYER_STATUS_ATTACK_DEFERRED:
            return "Combat character \(character) attack deferred"
        case U3_COMBAT_PLAYER_STATUS_UNSUPPORTED:
            return "Combat CONS \(session.screenResourceID) command deferred"
        default:
            return "Combat CONS \(session.screenResourceID) command deferred"
        }
    }

    private func describeCombatMonsterTurn(_ result: u3_combat_monster_turn_result) -> String {
        let monster = result.monster == UInt8(U3_COMBAT_NO_SLOT) ? 0 : result.monster + 1
        let target = result.target_character == UInt8(U3_COMBAT_NO_SLOT) ? 0 : result.target_character + 1

        switch Int32(result.status) {
        case U3_COMBAT_MONSTER_TURN_STATUS_MOVED:
            return "Monster \(monster) move \(result.action_result.moved_to_x),\(result.action_result.moved_to_y)"
        case U3_COMBAT_MONSTER_TURN_STATUS_ATTACK_MISSED:
            return "Monster \(monster) missed character \(target)"
        case U3_COMBAT_MONSTER_TURN_STATUS_ATTACK_HIT:
            if result.action_result.character_died != 0 {
                return "Monster \(monster) hit character \(target) damage \(result.action_result.damage_amount) defeated"
            }
            if result.action_result.poisoned != 0 {
                return "Monster \(monster) hit character \(target) damage \(result.action_result.damage_amount) poisoned"
            }
            if result.action_result.pilfered != 0 {
                return "Monster \(monster) hit character \(target) damage \(result.action_result.damage_amount) pilfered"
            }
            return "Monster \(monster) hit character \(target) damage \(result.action_result.damage_amount)"
        case U3_COMBAT_MONSTER_TURN_STATUS_SHOT:
            if result.action_result.projectile_missed != 0 {
                return "Monster \(monster) shot missed"
            }
            return "Monster \(monster) shot character \(target) damage \(result.action_result.damage_amount)"
        case U3_COMBAT_MONSTER_TURN_STATUS_SPELL_DEFERRED:
            return "Monster \(monster) spell deferred character \(target)"
        case U3_COMBAT_MONSTER_TURN_STATUS_NO_ACTION:
            fallthrough
        default:
            return "Monster no action"
        }
    }

    private func consumeDungeonInput(_ event: u3_input_event, session: inout ShellLocationSession) -> String {
        if let pendingCommand = pendingDungeonInteractionCommand {
            return consumeDungeonInteractionSelection(event, session: &session, command: pendingCommand)
        }

        if Self.normalizedKeyboardCommand(event.command) == UInt16(UInt8(ascii: "I")) {
            return consumeIgniteCommand(session: &session)
        }

        if Self.normalizedKeyboardCommand(event.command) == UInt16(UInt8(ascii: "G")) {
            return beginDungeonInteraction(session: &session, command: event.command)
        }

        var dungeonResult = u3_dungeon_result()
        guard resourceAdapter.moveDungeonSession(
            &session,
            command: event.command,
            result: &dungeonResult
        ) else {
            return "Dungeon MAPS \(session.descriptor.resource_id) command \(inputAdapter.describeKey(event.command)) deferred"
        }

        var locationResult = makeLocationResult(session: session, dungeonResult: dungeonResult)
        guard applyLocationMoveTransaction(
            session: session,
            result: &locationResult
        ) else {
            return "Location transition failed"
        }

        var postTurnResult = u3_dungeon_post_turn_result()
        var specialEffectResult = ShellDungeonSpecialEffectResult(
            effect: u3_dungeon_special_effect_result(),
            documentMutated: false,
            sessionMutated: false,
            message: nil
        )
        var interactionResult = ShellDungeonInteractionResult(
            interaction: u3_dungeon_interaction_result(),
            documentMutated: false,
            sessionMutated: false,
            message: nil
        )
        if !dungeonResult.exited {
            let rolls = dungeonRollProvider(session.descriptor.dungeon_level)
            postTurnResult = resourceAdapter.applyDungeonPostTurn(
                &session,
                documentData: currentSaveDocument,
                encounterRoll: rolls.encounter,
                monsterRoll: rolls.monster
            )
            if postTurnResult.encounter_requested != 0 {
                return activateCombatSession(
                    encounter: postTurnResult,
                    sourceSession: session,
                    prefix: "Dungeon command \(describeDungeonCommand(event.command)) \(describeDungeonPosition(session))\(describeLocationTurnDelta(locationResult))"
                )
            }
            var documentData = currentSaveDocument
            specialEffectResult = resourceAdapter.applyDungeonSpecialEffect(
                &session,
                documentData: &documentData,
                disarmRoll: UInt16.random(in: 0...255),
                gremlinRoll: UInt16.random(in: 0...255),
                trapDamageRoll: UInt16.random(in: 0...255)
            )
            if specialEffectResult.documentMutated {
                currentSaveDocument = documentData
                hasUnsavedChanges = true
            }
            if specialEffectResult.effect.status == UInt8(U3_DUNGEON_SPECIAL_STATUS_UNSUPPORTED) {
                var interactionDocumentData = currentSaveDocument
                interactionResult = resourceAdapter.applyDungeonInteraction(
                    &session,
                    documentData: &interactionDocumentData,
                    command: 0,
                    selectedActiveSlot: 0,
                    chestTrapRoll: 0,
                    chestGoldRoll: 0
                )
                if interactionResult.interaction.requires_selection != 0 {
                    pendingDungeonInteractionCommand = 0
                }
                if interactionResult.documentMutated {
                    currentSaveDocument = interactionDocumentData
                    hasUnsavedChanges = true
                }
                if interactionResult.interaction.handled != 0 {
                    specialEffectResult = ShellDungeonSpecialEffectResult(
                        effect: u3_dungeon_special_effect_result(),
                        documentMutated: false,
                        sessionMutated: false,
                        message: nil
                    )
                }
            }
        }
        if interactionResult.interaction.sound_id != 0 {
            _ = audioAdapter.enqueueSound(Int32(interactionResult.interaction.sound_id))
        } else if specialEffectResult.effect.sound_id != 0 {
            _ = audioAdapter.enqueueSound(Int32(specialEffectResult.effect.sound_id))
        } else if dungeonResult.blocked {
            _ = audioAdapter.enqueueSound(Int32(U3_AUDIO_SOUND_BUMP))
        } else if dungeonResult.moved || dungeonResult.turned || dungeonResult.level_changed {
            _ = audioAdapter.enqueueSound(Int32(U3_AUDIO_SOUND_STEP))
        }

        if dungeonResult.exited {
            return "Returned to Sosaria \(overworldState.x),\(overworldState.y)\(describeLocationTurnDelta(locationResult))"
        }

        activeLocationSession = session
        if dungeonResult.needs_redraw || postTurnResult.light_decremented != 0 || specialEffectResult.sessionMutated || interactionResult.sessionMutated {
            renderFrame = session.frame
            let locations = locationProvider.snapshot()
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: session.status
            )
        }

        let interactionDescription = describeDungeonInteraction(interactionResult)

        if dungeonResult.blocked {
            return "Dungeon blocked \(describeDungeonCommand(event.command)) \(describeDungeonPosition(session))\(describeLocationTurnDelta(locationResult))\(describeDungeonPostTurn(postTurnResult))\(describeDungeonSpecialEffect(specialEffectResult))\(interactionDescription)"
        }
        if dungeonResult.invalid {
            return "Dungeon invalid \(describeDungeonCommand(event.command)) \(describeDungeonPosition(session))\(describeLocationTurnDelta(locationResult))\(describeDungeonPostTurn(postTurnResult))\(describeDungeonSpecialEffect(specialEffectResult))\(interactionDescription)"
        }
        if dungeonResult.level_changed {
            return "Dungeon level \(describeDungeonPosition(session))\(describeLocationTurnDelta(locationResult))\(describeDungeonPostTurn(postTurnResult))\(describeDungeonSpecialEffect(specialEffectResult))\(interactionDescription)"
        }
        if dungeonResult.turned {
            return "Dungeon turn \(describeDungeonCommand(event.command)) \(describeDungeonPosition(session))\(describeLocationTurnDelta(locationResult))\(describeDungeonPostTurn(postTurnResult))\(describeDungeonSpecialEffect(specialEffectResult))\(interactionDescription)"
        }
        if dungeonResult.moved {
            return "Dungeon move \(describeDungeonCommand(event.command)) \(describeDungeonPosition(session))\(describeLocationTurnDelta(locationResult))\(describeDungeonPostTurn(postTurnResult))\(describeDungeonSpecialEffect(specialEffectResult))\(interactionDescription)"
        }
        return "Dungeon command \(describeDungeonCommand(event.command)) \(describeDungeonPosition(session))\(describeLocationTurnDelta(locationResult))\(describeDungeonPostTurn(postTurnResult))\(describeDungeonSpecialEffect(specialEffectResult))\(interactionDescription)"
    }

    private func beginDungeonInteraction(session: inout ShellLocationSession, command: UInt16) -> String {
        var documentData = currentSaveDocument
        let result = resourceAdapter.applyDungeonInteraction(
            &session,
            documentData: &documentData,
            command: command,
            selectedActiveSlot: 0,
            chestTrapRoll: 0,
            chestGoldRoll: 0
        )
        if result.interaction.requires_selection != 0 {
            pendingDungeonInteractionCommand = command
        }
        let turnDescription = applyDungeonInteractionTurnIfNeeded(
            command: command,
            session: &session,
            documentData: documentData,
            result: result
        )
        if activeCombatSession == nil {
            activeLocationSession = session
        }
        return "\(describeDungeonInteraction(result))\(turnDescription ?? "")"
            .trimmingCharacters(in: .whitespaces)
    }

    private func consumeDungeonInteractionSelection(
        _ event: u3_input_event,
        session: inout ShellLocationSession,
        command: UInt16
    ) -> String {
        let selectedSlot = Self.dungeonInteractionSelection(from: event.command)
        let chestRolls = dungeonChestRollProvider()
        var documentData = currentSaveDocument
        let result = resourceAdapter.applyDungeonInteraction(
            &session,
            documentData: &documentData,
            command: command,
            selectedActiveSlot: selectedSlot,
            chestTrapRoll: chestRolls.trap,
            chestGoldRoll: chestRolls.gold
        )

        if result.interaction.requires_selection != 0 {
            activeLocationSession = session
            return "Dungeon interaction: choose character"
        }

        pendingDungeonInteractionCommand = nil
        if result.sessionMutated {
            renderFrame = session.frame
            let locations = locationProvider.snapshot()
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: session.status
            )
        }
        if result.interaction.sound_id != 0 {
            _ = audioAdapter.enqueueSound(Int32(result.interaction.sound_id))
        }
        let turnDescription = applyDungeonInteractionTurnIfNeeded(
            command: command,
            session: &session,
            documentData: documentData,
            result: result
        )
        if result.documentMutated, turnDescription == nil {
            currentSaveDocument = documentData
            hasUnsavedChanges = true
        }
        if activeCombatSession == nil {
            activeLocationSession = session
        }
        return "\(describeDungeonInteraction(result))\(turnDescription ?? "")"
            .trimmingCharacters(in: .whitespaces)
    }

    private func applyDungeonInteractionTurnIfNeeded(
        command: UInt16,
        session: inout ShellLocationSession,
        documentData: Data?,
        result: ShellDungeonInteractionResult
    ) -> String? {
        guard Self.normalizedKeyboardCommand(command) == UInt16(UInt8(ascii: "G")),
              Self.dungeonInteractionConsumesTurn(result),
              result.interaction.requires_selection == 0 else {
            return nil
        }

        if result.documentMutated, let documentData {
            currentSaveDocument = documentData
        }

        var locationResult = u3_location_move_result()
        locationResult.handled = 1
        locationResult.x = session.descriptor.x
        locationResult.y = session.descriptor.y
        locationResult.status = UInt8(U3_LOCATION_MOVE_STATUS_MOVED)
        guard applyLocationMoveTransaction(session: session, result: &locationResult) else {
            return " turn failed"
        }

        let rolls = dungeonRollProvider(session.descriptor.dungeon_level)
        let postTurnResult = resourceAdapter.applyDungeonPostTurn(
            &session,
            documentData: currentSaveDocument,
            encounterRoll: rolls.encounter,
            monsterRoll: rolls.monster
        )
        if postTurnResult.light_decremented != 0 {
            renderFrame = session.frame
            let locations = locationProvider.snapshot()
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: session.status
            )
        }
        if postTurnResult.encounter_requested != 0 {
            return activateCombatSession(
                encounter: postTurnResult,
                sourceSession: session,
                prefix: describeLocationTurnDelta(locationResult)
            )
        }
        return "\(describeLocationTurnDelta(locationResult))\(describeDungeonPostTurn(postTurnResult))"
    }

    private func consumeIgniteCommand(session: inout ShellLocationSession) -> String {
        guard var documentData = currentSaveDocument else {
            return "Ignite failed: no current game"
        }

        let result = resourceAdapter.igniteTorch(documentData: &documentData)
        guard result.ignited != 0 else {
            return describeIgniteFailure(result)
        }

        currentSaveDocument = documentData
        hasUnsavedChanges = true
        session.descriptor.light = result.light
        resourceAdapter.refreshLocationSessionFrame(&session)
        activeLocationSession = session
        renderFrame = session.frame
        let locations = locationProvider.snapshot()
        resourceStatus = Self.describeResourceStatus(
            locations: locations,
            validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
            renderStatus: session.status
        )
        _ = audioAdapter.enqueueSound(Int32(U3_AUDIO_SOUND_TORCH_IGNITE))
        return "Ignite roster \(result.roster_id) torches \(result.torch_count_after) light \(session.descriptor.light)"
    }

    private func makeLocationResult(
        session: ShellLocationSession,
        dungeonResult: u3_dungeon_result
    ) -> u3_location_move_result {
        var result = u3_location_move_result()
        result.handled = 1
        result.moved = dungeonResult.moved ? 1 : 0
        result.blocked = (dungeonResult.blocked || dungeonResult.invalid) ? 1 : 0
        result.exit_requested = dungeonResult.exited ? 1 : 0
        result.x = session.descriptor.x
        result.y = session.descriptor.y
        result.redraw = dungeonResult.needs_redraw ? 1 : 0
        if dungeonResult.exited {
            result.status = UInt8(U3_LOCATION_MOVE_STATUS_EXIT_REQUESTED)
        } else if dungeonResult.blocked || dungeonResult.invalid {
            result.status = UInt8(U3_LOCATION_MOVE_STATUS_BLOCKED)
        } else {
            result.status = UInt8(U3_LOCATION_MOVE_STATUS_MOVED)
        }
        return result
    }

    private func activateLocationSession(request: u3_location_transition_result) -> String {
        let locations = locationProvider.snapshot()
        switch resourceAdapter.loadLocationSession(
            resourceRootPath: locations.resourceRootPath,
            request: request
        ) {
        case .success(let session):
            guard var documentData = currentSaveDocument else {
                return "Location entry failed: no current game"
            }
            var appliedRequest = request
            guard locationTransitionAdapter.applyEntry(to: &documentData, request: &appliedRequest) else {
                return "Location entry failed: invalid current state"
            }

            currentSaveDocument = documentData
            hasUnsavedChanges = true
            activeLocationSession = session
            activeCombatSession = nil
            renderFrame = session.frame
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: session.status
            )
            if session.descriptor.destination_kind == U3_LOCATION_KIND_DUNGEON {
                return "Entered dungeon index \(session.descriptor.location_index) MAPS \(session.descriptor.resource_id) return \(session.descriptor.return_x),\(session.descriptor.return_y) level \(session.descriptor.dungeon_level) start \(session.descriptor.x),\(session.descriptor.y) heading \(session.descriptor.heading) moves \(appliedRequest.move_counter_after)"
            }
            return "Entered town index \(session.descriptor.location_index) MAPS \(session.descriptor.resource_id) return \(session.descriptor.return_x),\(session.descriptor.return_y) start \(session.descriptor.x),\(session.descriptor.y) heading \(session.descriptor.heading) moves \(appliedRequest.move_counter_after)"
        case .failure(let status):
            return status
        }
    }

    private func activateCombatSession(
        encounter: u3_dungeon_post_turn_result,
        sourceSession: ShellLocationSession,
        prefix: String
    ) -> String {
        let locations = locationProvider.snapshot()
        switch combatSessionLoader(locations.resourceRootPath, encounter, sourceSession, currentSaveDocument) {
        case .success(var session):
            session.activeCharacter = playableCombatCharacter(in: session, startingAt: 0) ?? 0
            activeCombatSession = session
            activeLocationSession = nil
            pendingCombatAttackCharacter = nil
            pendingDungeonInteractionCommand = nil
            awaitingTalkDirection = false
            renderFrame = session.frame
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: session.status
            )
            return "\(prefix) entered combat CONS \(session.screenResourceID) monster \(session.monsterType) marker \(session.markerTile)"
        case .failure(let status):
            activeCombatSession = nil
            activeLocationSession = sourceSession
            renderFrame = sourceSession.frame
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: sourceSession.status
            )
            return "\(prefix) combat entry failed: \(status)"
        }
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

    private func applyLocationMoveTransaction(
        session: ShellLocationSession,
        result: inout u3_location_move_result
    ) -> Bool {
        guard var documentData = currentSaveDocument else {
            return false
        }

        guard locationTransitionAdapter.applyMove(
            to: &documentData,
            session: session,
            result: &result
        ) else {
            return false
        }

        if result.exit_requested != 0 {
            var restoredState = u3_overworld_state()
            let overworldSmoke = resourceAdapter.buildOverworldSmoke(
                documentData: documentData,
                state: &restoredState
            )
            guard let restoredMap = overworldSmoke.map,
                  restoredState.x == session.descriptor.return_x,
                  restoredState.y == session.descriptor.return_y else {
                return false
            }

            let locations = locationProvider.snapshot()
            currentSaveDocument = documentData
            hasUnsavedChanges = true
            activeLocationSession = nil
            overworldState = restoredState
            overworldMapData = restoredMap
            renderFrame = overworldSmoke.frame
            resourceStatus = Self.describeResourceStatus(
                locations: locations,
                validation: resourceAdapter.validateMainResources(resourceRootPath: locations.resourceRootPath),
                renderStatus: overworldSmoke.status
            )
            return true
        }

        currentSaveDocument = documentData
        hasUnsavedChanges = true
        return true
    }

    private func applyCurrentSaveDocument(_ document: Data, locations: ShellLocationSnapshot, statusPrefix: String) {
        activeLocationSession = nil
        activeCombatSession = nil
        awaitingTalkDirection = false
        pendingCombatAttackCharacter = nil
        pendingDungeonInteractionCommand = nil
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

#if DEBUG
    func debugSetCurrentDungeonTile(_ tile: UInt8) -> Bool {
        guard var session = activeLocationSession,
              session.descriptor.destination_kind == U3_LOCATION_KIND_DUNGEON else {
            return false
        }
        let offset = (Int(session.descriptor.dungeon_level) * Int(U3_DUNGEON_WIDTH) * Int(U3_DUNGEON_HEIGHT)) +
            (Int(session.descriptor.y) * Int(U3_DUNGEON_WIDTH)) +
            Int(session.descriptor.x)
        guard offset >= 0 && offset < session.mapData.count else {
            return false
        }
        session.mapData[offset] = tile
        resourceAdapter.refreshLocationSessionFrame(&session)
        activeLocationSession = session
        renderFrame = session.frame
        return true
    }

    func debugCurrentDungeonTile() -> UInt8? {
        guard let session = activeLocationSession,
              session.descriptor.destination_kind == U3_LOCATION_KIND_DUNGEON else {
            return nil
        }
        let offset = (Int(session.descriptor.dungeon_level) * Int(U3_DUNGEON_WIDTH) * Int(U3_DUNGEON_HEIGHT)) +
            (Int(session.descriptor.y) * Int(U3_DUNGEON_WIDTH)) +
            Int(session.descriptor.x)
        guard offset >= 0 && offset < session.mapData.count else {
            return nil
        }
        return session.mapData[offset]
    }

    func debugActiveCombatStatus() -> String? {
        activeCombatSession?.status
    }

    func debugActiveCombatSourceDungeonTile() -> UInt8? {
        guard let session = activeCombatSession?.sourceLocationSession else {
            return nil
        }
        let offset = (Int(session.descriptor.dungeon_level) * Int(U3_DUNGEON_WIDTH) * Int(U3_DUNGEON_HEIGHT)) +
            (Int(session.descriptor.y) * Int(U3_DUNGEON_WIDTH)) +
            Int(session.descriptor.x)
        guard offset >= 0 && offset < session.mapData.count else {
            return nil
        }
        return session.mapData[offset]
    }

    func debugActiveLocationSession() -> ShellLocationSession? {
        activeLocationSession
    }

    func debugInstallCombatSession(_ session: ShellCombatSession) -> Bool {
        activeCombatSession = session
        activeLocationSession = nil
        pendingCombatAttackCharacter = nil
        pendingDungeonInteractionCommand = nil
        awaitingTalkDirection = false
        renderFrame = session.frame
        return true
    }

    func debugCurrentSaveDocument() -> Data? {
        currentSaveDocument
    }

    func debugConfigureActiveCombat(
        monsterX: UInt8,
        monsterY: UInt8,
        monsterHP: UInt8,
        characterX: UInt8,
        characterY: UInt8,
        characterHP: UInt16
    ) -> Bool {
        guard var session = activeCombatSession else {
            return false
        }

        session.combatState.monster_x.0 = monsterX
        session.combatState.monster_y.0 = monsterY
        session.combatState.monster_hp.0 = monsterHP
        session.combatState.monster_tile.0 = 0
        session.combatState.character_x.0 = characterX
        session.combatState.character_y.0 = characterY
        session.combatState.character_hp.0 = characterHP
        session.combatState.character_status.0 = UInt8(ascii: "G")
        session.combatState.character_shape.0 = 0x80
        session.combatState.character_tile.0 = 0
        withUnsafeMutableBytes(of: &session.combatState.tile_array) { buffer in
            for index in 0..<Int(U3_RENDER_TILE_COUNT) {
                buffer[index] = 0
            }
            buffer[(Int(monsterY) * 11) + Int(monsterX)] = session.monsterType
            buffer[(Int(characterY) * 11) + Int(characterX)] = session.combatState.character_shape.0
        }
        session.activeCharacter = 0
        session.nextMonster = 0
        resourceAdapter.refreshCombatSessionFrame(&session)
        activeCombatSession = session
        renderFrame = session.frame
        return true
    }
#endif

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

    private func describeLocationTurnDelta(_ result: u3_location_move_result) -> String {
        guard result.turn_applied != 0 else {
            return ""
        }

        return " moves \(result.move_counter_after)"
    }

    private func describeDungeonCommand(_ command: UInt16) -> String {
        let normalizedCommand = Self.normalizedKeyboardCommand(command)

        switch Int32(normalizedCommand) {
        case U3_OVERWORLD_COMMAND_NORTH:
            return "Forward"
        case U3_OVERWORLD_COMMAND_SOUTH:
            return "Retreat"
        case U3_OVERWORLD_COMMAND_WEST:
            return "Left"
        case U3_OVERWORLD_COMMAND_EAST:
            return "Right"
        case Int32(UInt8(ascii: "D")):
            return "Descend"
        case Int32(UInt8(ascii: "K")):
            return "Klimb"
        case Int32(UInt8(ascii: " ")):
            return "Pass"
        default:
            return inputAdapter.describeKey(command)
        }
    }

    private func describeDungeonPosition(_ session: ShellLocationSession) -> String {
        "pos \(session.descriptor.x),\(session.descriptor.y) heading \(session.descriptor.heading) level \(session.descriptor.dungeon_level) light \(session.descriptor.light)"
    }

    private func describeDungeonPostTurn(_ result: u3_dungeon_post_turn_result) -> String {
        guard result.handled != 0 else {
            return ""
        }
        guard result.encounter_requested != 0 else {
            return " passive"
        }
        return " encounter monster \(result.monster_type) CONS \(result.combat_screen_resource_id) marker \(result.marker_tile)"
    }

    private func describeDungeonSpecialEffect(_ result: ShellDungeonSpecialEffectResult) -> String {
        let effect = result.effect
        guard effect.handled != 0, effect.current_tile != 0 else {
            return ""
        }

        switch Int32(effect.status) {
        case U3_DUNGEON_SPECIAL_STATUS_WIND:
            return " special wind light \(effect.light_after)"
        case U3_DUNGEON_SPECIAL_STATUS_TRAP_DISARMED:
            return " special trap disarmed roster \(effect.roster_id)"
        case U3_DUNGEON_SPECIAL_STATUS_TRAP_DAMAGE:
            return " special trap damage \(effect.damage_per_living_member) hit \(effect.damaged_living_members) killed \(effect.killed_members)"
        case U3_DUNGEON_SPECIAL_STATUS_GREMLINS:
            return " special gremlins roster \(effect.roster_id) food \(effect.food_after)"
        case U3_DUNGEON_SPECIAL_STATUS_WRITING:
            if let message = result.message {
                return " special \(message)"
            }
            return " special writing"
        case U3_DUNGEON_SPECIAL_STATUS_NO_ELIGIBLE_CHARACTER:
            return " special no eligible character"
        case U3_DUNGEON_SPECIAL_STATUS_INVALID_INPUT:
            return " special invalid"
        default:
            return " special unsupported tile \(effect.current_tile)"
        }
    }

    private func describeDungeonInteraction(_ result: ShellDungeonInteractionResult) -> String {
        let interaction = result.interaction
        guard interaction.handled != 0 else {
            return ""
        }

        switch Int32(interaction.status) {
        case U3_DUNGEON_INTERACTION_STATUS_TIME_LORD:
            return " interaction \(result.message ?? "Time Lord")"
        case U3_DUNGEON_INTERACTION_STATUS_SELECTION_REQUIRED:
            return " interaction \(result.message ?? "choose character")"
        case U3_DUNGEON_INTERACTION_STATUS_CANCELLED:
            return " interaction cancelled"
        case U3_DUNGEON_INTERACTION_STATUS_INVALID_CHARACTER:
            return " interaction invalid character"
        case U3_DUNGEON_INTERACTION_STATUS_INCAPACITATED:
            return " interaction incapacitated slot \(interaction.active_slot)"
        case U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_POISON:
            return " interaction fountain poison roster \(interaction.roster_id) status \(characterCode(interaction.status_after))"
        case U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_HEAL:
            return " interaction fountain heal roster \(interaction.roster_id) HP \(interaction.hit_points_after)"
        case U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_DAMAGE:
            return " interaction fountain damage roster \(interaction.roster_id) HP \(interaction.hit_points_after)"
        case U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_CURE:
            return " interaction fountain cure roster \(interaction.roster_id) status \(characterCode(interaction.status_after))"
        case U3_DUNGEON_INTERACTION_STATUS_MARK:
            return " interaction mark roster \(interaction.roster_id) marks \(interaction.mark_after) HP \(interaction.hit_points_after)"
        case U3_DUNGEON_INTERACTION_STATUS_CHEST_OPENED:
            return " interaction chest roster \(interaction.roster_id) gold \(interaction.gold_after) added \(interaction.gold_added)"
        case U3_DUNGEON_INTERACTION_STATUS_CHEST_TRAP_DEFERRED:
            return " interaction chest trap deferred"
        case U3_DUNGEON_INTERACTION_STATUS_INVALID_INPUT:
            return " interaction invalid"
        default:
            return " interaction unsupported tile \(interaction.current_tile)"
        }
    }

    private func characterCode(_ value: UInt8) -> String {
        guard value >= 32 && value <= 126,
              let scalar = UnicodeScalar(Int(value)) else {
            return "#\(value)"
        }
        return String(Character(scalar))
    }

    private func describeIgniteFailure(_ result: u3_party_ignite_result) -> String {
        switch Int32(result.reason) {
        case U3_PARTY_IGNITE_NO_TORCH:
            return "Ignite failed: no torch"
        case U3_PARTY_IGNITE_INVALID_CHARACTER:
            return "Ignite failed: invalid character"
        default:
            return "Ignite failed"
        }
    }

    private static func normalizedKeyboardCommand(_ command: UInt16) -> UInt16 {
        if command >= UInt16(UInt8(ascii: "a")) && command <= UInt16(UInt8(ascii: "z")) {
            return command - 32
        }
        return command
    }

    private static func combatDirection(for command: UInt16) -> (dx: Int8, dy: Int8)? {
        switch normalizedKeyboardCommand(command) {
        case UInt16(U3_COMBAT_COMMAND_NORTH), UInt16(UInt8(ascii: "N")):
            return (0, -1)
        case UInt16(U3_COMBAT_COMMAND_SOUTH), UInt16(UInt8(ascii: "S")):
            return (0, 1)
        case UInt16(U3_COMBAT_COMMAND_WEST), UInt16(UInt8(ascii: "W")):
            return (-1, 0)
        case UInt16(U3_COMBAT_COMMAND_EAST), UInt16(UInt8(ascii: "E")):
            return (1, 0)
        default:
            return nil
        }
    }

    private static func dungeonInteractionSelection(from command: UInt16) -> UInt8 {
        if command >= UInt16(UInt8(ascii: "1")) && command <= UInt16(UInt8(ascii: "4")) {
            return UInt8(command - UInt16(UInt8(ascii: "0")))
        }
        if command == UInt16(UInt8(ascii: "0")) || command == 27 {
            return UInt8(U3_PARTY_ACTIVE_SLOT_COUNT + 1)
        }
        return 0
    }

    private static func dungeonInteractionConsumesTurn(_ result: ShellDungeonInteractionResult) -> Bool {
        guard result.interaction.handled != 0 else {
            return false
        }

        switch Int32(result.interaction.status) {
        case U3_DUNGEON_INTERACTION_STATUS_CHEST_OPENED,
             U3_DUNGEON_INTERACTION_STATUS_CHEST_TRAP_DEFERRED:
            return true
        default:
            return false
        }
    }

    private static func makeDungeonRolls(level: UInt8) -> (encounter: UInt16, monster: UInt16) {
        let encounterMax = UInt16(0x82 + UInt16(level))
        let monsterMax = UInt16(level) + 2
        return (
            UInt16.random(in: 0...encounterMax),
            UInt16.random(in: 0...monsterMax)
        )
    }

    private static func makeDungeonChestRolls() -> (trap: UInt16, gold: UInt16) {
        (
            UInt16.random(in: 0...255),
            UInt16.random(in: 0...100)
        )
    }

    private static func makeCombatPlayerRolls(strength: UInt8) -> CombatPlayerRolls {
        (
            hitChance: UInt8.random(in: 0...255),
            hitDexterity: UInt8.random(in: 0...99),
            damage: UInt8.random(in: 0...(strength | 1))
        )
    }

    private static func makeCombatMonsterRolls() -> CombatMonsterRolls {
        (
            shootChoice: UInt8.random(in: 0...255),
            magicChoice: UInt8.random(in: 0...255),
            magicTarget: UInt8.random(in: 0..<UInt8(U3_COMBAT_CHARACTER_COUNT)),
            projectileHit: UInt8.random(in: 0..<UInt8(U3_COMBAT_CHARACTER_COUNT)),
            poison: UInt8.random(in: 0...255),
            pilferBranch: UInt8.random(in: 0...255),
            pilferItem: UInt8.random(in: 0...255),
            armourHit: UInt8.random(in: 0...15),
            damage: UInt8.random(in: 0...255),
            exodusDamage: UInt8.random(in: 0...3)
        )
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
