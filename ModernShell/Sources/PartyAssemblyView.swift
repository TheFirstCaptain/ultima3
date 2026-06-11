import SwiftUI

struct PartyAssemblyView: View {
    let presentation: ShellPartyAssemblyPresentation
    let onAccept: ([UInt8]) -> Void
    let onCancel: () -> Void

    @State private var selectedRosterIDs: [UInt8] = []
    @State private var validationMessage: String

    init(
        presentation: ShellPartyAssemblyPresentation,
        onAccept: @escaping ([UInt8]) -> Void,
        onCancel: @escaping () -> Void
    ) {
        self.presentation = presentation
        self.onAccept = onAccept
        self.onCancel = onCancel
        _validationMessage = State(initialValue: presentation.message)
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            Text("Assemble Party")
                .font(.headline)

            Text(validationMessage)
                .foregroundColor(canAccept ? .secondary : .red)

            if presentation.options.isEmpty {
                Spacer(minLength: 0)
            } else {
                VStack(alignment: .leading, spacing: 8) {
                    ForEach(presentation.options) { option in
                        rosterRow(option)
                    }
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }

            HStack {
                Text(selectionStatus)
                    .monospacedDigit()
                    .foregroundColor(.secondary)
                Spacer()
                Button("Cancel") {
                    onCancel()
                }
                .keyboardShortcut(.cancelAction)
                Button("Assemble") {
                    if canAccept {
                        onAccept(selectedRosterIDs)
                    } else {
                        validationMessage = "Select one to four members"
                    }
                }
                .disabled(!canAccept)
                .keyboardShortcut(.defaultAction)
            }
        }
        .padding(20)
        .frame(minWidth: 520, minHeight: 360)
    }

    private func rosterRow(_ option: ShellPartyRosterOption) -> some View {
        let isSelected = selectedRosterIDs.contains(option.id)
        return HStack(spacing: 10) {
            Toggle("", isOn: Binding(
                get: { selectedRosterIDs.contains(option.id) },
                set: { enabled in
                    setSelection(option.id, enabled: enabled)
                }
            ))
            .labelsHidden()

            Text(orderLabel(for: option.id, selected: isSelected))
                .monospacedDigit()
                .frame(width: 24, alignment: .trailing)
                .foregroundColor(isSelected ? .primary : .secondary)

            VStack(alignment: .leading, spacing: 2) {
                Text("\(option.id). \(option.name)")
                    .lineLimit(1)
                Text(option.detail)
                    .font(.caption)
                    .foregroundColor(.secondary)
                    .lineLimit(1)
            }

            Spacer(minLength: 0)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var canAccept: Bool {
        presentation.canAssemble && selectedRosterIDs.count >= 1 && selectedRosterIDs.count <= 4
    }

    private var selectionStatus: String {
        "\(selectedRosterIDs.count)/4 selected"
    }

    private func orderLabel(for rosterID: UInt8, selected: Bool) -> String {
        guard selected,
              let index = selectedRosterIDs.firstIndex(of: rosterID) else {
            return "-"
        }

        return "\(index + 1)"
    }

    private func setSelection(_ rosterID: UInt8, enabled: Bool) {
        if enabled {
            guard !selectedRosterIDs.contains(rosterID) else {
                return
            }
            guard selectedRosterIDs.count < 4 else {
                validationMessage = "Select one to four members"
                return
            }
            selectedRosterIDs.append(rosterID)
        } else {
            selectedRosterIDs.removeAll { $0 == rosterID }
        }

        validationMessage = selectedRosterIDs.isEmpty ? "Select one to four members" : presentation.message
    }
}
