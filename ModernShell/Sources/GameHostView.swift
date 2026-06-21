import AppKit
import Combine
import Ultima3Core

final class GameHostView: NSView {
    private let shellState: ShellSmokeState
    private let renderer = AppKitRenderAdapter()
    private var cancellable: AnyCancellable?
    private var renderCancellable: AnyCancellable?
    private var tickCancellable: AnyCancellable?

    init(shellState: ShellSmokeState) {
        self.shellState = shellState
        super.init(frame: .zero)
        wantsLayer = true
        layer?.contentsScale = NSScreen.main?.backingScaleFactor ?? 2
        cancellable = shellState.$lastCommand.sink { [weak self] _ in
            self?.needsDisplay = true
        }
        renderCancellable = shellState.$renderFrame.sink { [weak self] _ in
            self?.needsDisplay = true
        }
        tickCancellable = shellState.$tickStatus.sink { [weak self] _ in
            self?.needsDisplay = true
        }
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override var acceptsFirstResponder: Bool {
        true
    }

    override var isFlipped: Bool {
        true
    }

    override func keyDown(with event: NSEvent) {
        if let movementCommand = movementCommand(for: event) {
            shellState.submitOverworldCommand(movementCommand)
            return
        }

        guard let character = event.charactersIgnoringModifiers?.utf8.first else {
            return
        }

        let uppercased = character >= UInt8(ascii: "a") && character <= UInt8(ascii: "z")
            ? character - 32
            : character
        shellState.submitKeyboard(uppercased)
    }

    private func movementCommand(for event: NSEvent) -> UInt16? {
        switch event.keyCode {
        case 126, 91:
            return UInt16(U3_OVERWORLD_COMMAND_NORTH)
        case 125, 84:
            return UInt16(U3_OVERWORLD_COMMAND_SOUTH)
        case 123, 86:
            return UInt16(U3_OVERWORLD_COMMAND_WEST)
        case 124, 88:
            return UInt16(U3_OVERWORLD_COMMAND_EAST)
        default:
            return nil
        }
    }

    override func mouseDown(with event: NSEvent) {
        let point = convert(event.locationInWindow, from: nil)
        shellState.submitMouseDown(x: Int16(point.x), y: Int16(point.y))
    }

    override func draw(_ dirtyRect: NSRect) {
        renderer.draw(frame: shellState.renderFrame, in: bounds)
        drawStatusText()
    }

    private func drawStatusText() {
        let paragraph = NSMutableParagraphStyle()
        paragraph.alignment = .center
        paragraph.lineBreakMode = .byTruncatingTail

        let titleAttributes: [NSAttributedString.Key: Any] = [
            .font: NSFont.monospacedSystemFont(ofSize: 28, weight: .semibold),
            .foregroundColor: NSColor(calibratedRed: 0.92, green: 0.95, blue: 0.90, alpha: 1),
            .paragraphStyle: paragraph
        ]
        let statusAttributes: [NSAttributedString.Key: Any] = [
            .font: NSFont.monospacedSystemFont(ofSize: 12, weight: .regular),
            .foregroundColor: NSColor(calibratedRed: 0.66, green: 0.85, blue: 0.78, alpha: 1),
            .paragraphStyle: paragraph
        ]

        "Ultima III".draw(
            in: NSRect(x: 0, y: bounds.midY + 12, width: bounds.width, height: 40),
            withAttributes: titleAttributes
        )
        drawStatusLine("Last command: \(shellState.lastCommand) | \(shellState.tickStatus) | Core probe: \(shellState.coreHeadingProbe)", index: 0, attributes: statusAttributes)
        drawStatusLine(shellState.resourceStatus, index: 1, attributes: statusAttributes)
        let saveState = shellState.hasUnsavedChanges ? "Unsaved changes" : "Saved state unchanged"
        drawStatusLine("\(saveState) | \(shellState.saveStatus)", index: 2, attributes: statusAttributes)
    }

    private func drawStatusLine(_ text: String, index: Int, attributes: [NSAttributedString.Key: Any]) {
        text.draw(
            in: NSRect(
                x: 12,
                y: max(bounds.height - 102 + (CGFloat(index) * 22), 0),
                width: max(bounds.width - 24, 0),
                height: 18
            ),
            withAttributes: attributes
        )
    }
}
