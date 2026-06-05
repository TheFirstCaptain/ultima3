import AppKit
import Combine
import Ultima3Core

final class GameHostView: NSView {
    private let shellState: ShellSmokeState
    private let renderer = AppKitRenderAdapter()
    private var renderFrame = u3_render_make_synthetic_tile_frame()
    private var cancellable: AnyCancellable?

    init(shellState: ShellSmokeState) {
        self.shellState = shellState
        super.init(frame: .zero)
        wantsLayer = true
        layer?.contentsScale = NSScreen.main?.backingScaleFactor ?? 2
        cancellable = shellState.$lastCommand.sink { [weak self] _ in
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
        guard let character = event.charactersIgnoringModifiers?.utf8.first else {
            return
        }

        let uppercased = character >= UInt8(ascii: "a") && character <= UInt8(ascii: "z")
            ? character - 32
            : character
        shellState.submitKeyboard(uppercased)
    }

    override func mouseDown(with event: NSEvent) {
        let point = convert(event.locationInWindow, from: nil)
        shellState.submitMouseDown(x: Int16(point.x), y: Int16(point.y))
    }

    override func draw(_ dirtyRect: NSRect) {
        renderer.draw(frame: renderFrame, in: bounds)
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
            .font: NSFont.monospacedSystemFont(ofSize: 16, weight: .regular),
            .foregroundColor: NSColor(calibratedRed: 0.66, green: 0.85, blue: 0.78, alpha: 1),
            .paragraphStyle: paragraph
        ]

        "Ultima III".draw(
            in: NSRect(x: 0, y: bounds.midY + 12, width: bounds.width, height: 40),
            withAttributes: titleAttributes
        )
        "Last command: \(shellState.lastCommand) | Core probe: \(shellState.coreHeadingProbe)".draw(
            in: NSRect(x: 12, y: max(bounds.height - 84, 0), width: max(bounds.width - 24, 0), height: 28),
            withAttributes: statusAttributes
        )
    }
}
