import Ultima3Core

final class ShellInputAdapter {
    private var queue = u3_input_queue()

    init() {
        u3_input_queue_init(&queue)
    }

    func submitKeyboard(_ key: UInt8) -> String {
        guard u3_input_queue_push_keyboard(&queue, key) != 0 else {
            return "Input queue overflow"
        }
        return consumeNextDescription()
    }

    func submitMouseDown(x: Int16, y: Int16) -> String {
        guard u3_input_queue_push_mouse_down(&queue, x, y, UInt8(U3_INPUT_MOUSE_BUTTON_PRIMARY)) != 0 else {
            return "Input queue overflow"
        }
        return consumeNextDescription()
    }

    func submitMenuCommand(_ command: Int32) -> String {
        guard u3_input_queue_push_menu_command(&queue, UInt16(command)) != 0 else {
            return "Input queue overflow"
        }
        return consumeNextDescription()
    }

    func submitMacro(_ commands: String) -> String {
        let bytes = Array(commands.utf8)
        guard bytes.count <= Int(U3_INPUT_QUEUE_CAPACITY) else {
            return "Input queue overflow"
        }

        var enqueued = false
        bytes.withUnsafeBufferPointer { buffer in
            if let baseAddress = buffer.baseAddress {
                enqueued = u3_input_queue_push_macro_keys(
                    &queue,
                    UnsafeRawPointer(baseAddress).assumingMemoryBound(to: CChar.self),
                    UInt8(bytes.count)
                ) != 0
            }
        }

        guard enqueued else {
            return "Input queue empty"
        }
        return consumeNextDescription()
    }

    private func consumeNextDescription() -> String {
        var event = u3_input_event()

        guard u3_input_queue_pop(&queue, &event) != 0 else {
            return "Input queue empty"
        }

        switch Int32(event.kind) {
        case U3_INPUT_EVENT_KEYBOARD:
            return "Keyboard \(describeKey(event.command))"
        case U3_INPUT_EVENT_MOUSE_DOWN:
            return "Mouse \(event.x),\(event.y)"
        case U3_INPUT_EVENT_MENU_COMMAND:
            return "Menu \(describeMenuCommand(event.command))"
        case U3_INPUT_EVENT_MACRO_KEY:
            return "Macro \(describeKey(event.command))"
        default:
            return "Unknown input"
        }
    }

    private func describeMenuCommand(_ command: UInt16) -> String {
        switch Int32(command) {
        case U3_INPUT_MENU_NEW_GAME:
            return "New Game"
        case U3_INPUT_MENU_SAVE:
            return "Save"
        default:
            return "#\(command)"
        }
    }

    private func describeKey(_ command: UInt16) -> String {
        guard command >= 32 && command <= 126,
              let scalar = UnicodeScalar(Int(command)) else {
            return "#\(command)"
        }

        return String(Character(scalar))
    }
}
