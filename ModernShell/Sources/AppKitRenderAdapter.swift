import AppKit
import Ultima3Core

final class AppKitRenderAdapter {
    func draw(frame: u3_render_frame, in bounds: NSRect) {
        var frame = frame
        let commandCount = min(Int(frame.command_count), Int(U3_RENDER_MAX_COMMANDS))
        let scale = logicalScale(for: frame, in: bounds)
        let origin = NSPoint(
            x: bounds.midX - CGFloat(frame.logical_width) * scale * 0.5,
            y: bounds.midY - CGFloat(frame.logical_height) * scale * 0.5
        )

        withUnsafePointer(to: &frame.commands) { commandsPointer in
            commandsPointer.withMemoryRebound(to: u3_render_command.self, capacity: commandCount) { commands in
                for index in 0..<commandCount {
                    draw(command: commands[index], bounds: bounds, origin: origin, scale: scale)
                }
            }
        }
    }

    private func logicalScale(for frame: u3_render_frame, in bounds: NSRect) -> CGFloat {
        let horizontal = bounds.width / CGFloat(frame.logical_width)
        let vertical = bounds.height / CGFloat(frame.logical_height)
        return max(1, floor(min(horizontal, vertical)))
    }

    private func draw(command: u3_render_command, bounds: NSRect, origin: NSPoint, scale: CGFloat) {
        switch Int32(command.kind) {
        case U3_RENDER_COMMAND_CLEAR:
            drawClear(command, bounds: bounds)
        case U3_RENDER_COMMAND_RECT:
            drawRect(command, origin: origin, scale: scale)
        case U3_RENDER_COMMAND_TILE:
            drawTile(command, origin: origin, scale: scale)
        default:
            break
        }
    }

    private func drawClear(_ command: u3_render_command, bounds: NSRect) {
        nsColor(command.fill).setFill()
        bounds.fill()
    }

    private func drawRect(_ command: u3_render_command, origin: NSPoint, scale: CGFloat) {
        let rect = logicalRect(command, origin: origin, scale: scale)

        if command.fill.alpha > 0 {
            nsColor(command.fill).setFill()
            rect.fill()
        }

        if command.stroke.alpha > 0 {
            nsColor(command.stroke).setStroke()
            let path = NSBezierPath(rect: rect)
            path.lineWidth = max(1, scale * 0.08)
            path.stroke()
        }
    }

    private func drawTile(_ command: u3_render_command, origin: NSPoint, scale: CGFloat) {
        let rect = logicalRect(command, origin: origin, scale: scale).insetBy(dx: 1, dy: 1)
        nsColor(command.fill).setFill()
        rect.fill()

        nsColor(command.stroke).setStroke()
        let path = NSBezierPath(rect: rect)
        path.lineWidth = 1
        path.stroke()
    }

    private func logicalRect(_ command: u3_render_command, origin: NSPoint, scale: CGFloat) -> NSRect {
        NSRect(
            x: origin.x + CGFloat(command.x) * scale,
            y: origin.y + CGFloat(command.y) * scale,
            width: CGFloat(command.width) * scale,
            height: CGFloat(command.height) * scale
        )
    }

    private func nsColor(_ color: u3_render_color) -> NSColor {
        NSColor(
            calibratedRed: CGFloat(color.red) / 255,
            green: CGFloat(color.green) / 255,
            blue: CGFloat(color.blue) / 255,
            alpha: CGFloat(color.alpha) / 255
        )
    }
}
