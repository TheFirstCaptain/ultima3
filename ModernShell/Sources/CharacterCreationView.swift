import SwiftUI

struct CharacterCreationView: View {
    private let adapter = ShellCharacterCreationAdapter()
    let onAccept: (String) -> Void
    let onCancel: () -> Void

    @State private var name = "Iolo"
    @State private var race = UInt8(ascii: "H")
    @State private var characterClass = UInt8(ascii: "F")
    @State private var sex = UInt8(ascii: "M")
    @State private var strength = 15
    @State private var dexterity = 15
    @State private var intelligence = 10
    @State private var wisdom = 10
    @State private var validationMessage = "Candidate OK"
    @State private var validationIsValid = true
    @State private var pointStatus = "0 pts"

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            VStack(alignment: .leading, spacing: 12) {
                fieldRow("Name") {
                    TextField("", text: $name)
                        .textFieldStyle(.roundedBorder)
                        .frame(width: 220)
                }
                fieldRow("Race") {
                    Picker("", selection: $race) {
                        Text("Human").tag(UInt8(ascii: "H"))
                        Text("Elf").tag(UInt8(ascii: "E"))
                        Text("Dwarf").tag(UInt8(ascii: "D"))
                        Text("Bobbit").tag(UInt8(ascii: "B"))
                        Text("Fuzzy").tag(UInt8(ascii: "F"))
                    }
                    .labelsHidden()
                    .frame(width: 220)
                }
                fieldRow("Class") {
                    Picker("", selection: $characterClass) {
                        Text("Fighter").tag(UInt8(ascii: "F"))
                        Text("Cleric").tag(UInt8(ascii: "C"))
                        Text("Wizard").tag(UInt8(ascii: "W"))
                        Text("Thief").tag(UInt8(ascii: "T"))
                        Text("Paladin").tag(UInt8(ascii: "P"))
                        Text("Barbarian").tag(UInt8(ascii: "B"))
                        Text("Lark").tag(UInt8(ascii: "L"))
                        Text("Illusionist").tag(UInt8(ascii: "I"))
                        Text("Druid").tag(UInt8(ascii: "D"))
                        Text("Alchemist").tag(UInt8(ascii: "A"))
                        Text("Ranger").tag(UInt8(ascii: "R"))
                    }
                    .labelsHidden()
                    .frame(width: 220)
                }
                fieldRow("Sex") {
                    Picker("", selection: $sex) {
                        Text("Female").tag(UInt8(ascii: "F"))
                        Text("Male").tag(UInt8(ascii: "M"))
                        Text("Other").tag(UInt8(ascii: "O"))
                    }
                    .labelsHidden()
                    .frame(width: 220)
                }

                attributeRow("Strength", value: $strength)
                attributeRow("Dexterity", value: $dexterity)
                attributeRow("Intelligence", value: $intelligence)
                attributeRow("Wisdom", value: $wisdom)
            }
            .frame(maxWidth: .infinity, alignment: .leading)

            validationRow

            HStack {
                Spacer()
                Button("Cancel") {
                    onCancel()
                }
                .keyboardShortcut(.cancelAction)
                Button("Accept") {
                    let validation = currentValidation
                    validationMessage = validation.message
                    if validation.valid {
                        onAccept(validation.summary)
                    }
                }
                .keyboardShortcut(.defaultAction)
            }
        }
        .padding(20)
        .frame(minWidth: 500, minHeight: 390)
        .onChange(of: name) { _ in refreshValidationMessage() }
        .onChange(of: race) { _ in refreshValidationMessage() }
        .onChange(of: characterClass) { _ in refreshValidationMessage() }
        .onChange(of: sex) { _ in refreshValidationMessage() }
        .onChange(of: strength) { _ in refreshValidationMessage() }
        .onChange(of: dexterity) { _ in refreshValidationMessage() }
        .onChange(of: intelligence) { _ in refreshValidationMessage() }
        .onChange(of: wisdom) { _ in refreshValidationMessage() }
    }

    private func fieldRow<Content: View>(_ title: String, @ViewBuilder content: () -> Content) -> some View {
        HStack(spacing: 12) {
            Text(title)
                .frame(width: 112, alignment: .trailing)
            content()
            Spacer(minLength: 0)
        }
    }

    private func attributeRow(_ title: String, value: Binding<Int>) -> some View {
        fieldRow(title) {
            HStack(spacing: 10) {
                Text("\(value.wrappedValue)")
                    .monospacedDigit()
                    .frame(width: 34, alignment: .trailing)
                Stepper("", value: value, in: 5...25)
                    .labelsHidden()
                    .frame(width: 56)
                Spacer(minLength: 0)
            }
            .frame(width: 220, alignment: .leading)
        }
    }

    private var currentValidation: ShellCharacterValidationResult {
        adapter.validate(ShellCharacterDraft(
            name: name,
            race: race,
            characterClass: characterClass,
            sex: sex,
            strength: UInt8(strength),
            dexterity: UInt8(dexterity),
            intelligence: UInt8(intelligence),
            wisdom: UInt8(wisdom)
        ))
    }

    private var validationRow: some View {
        let color: Color = validationIsValid ? .secondary : .red
        return HStack {
            Text(validationMessage)
                .foregroundColor(color)
            Spacer()
            Text(pointStatus)
                .monospacedDigit()
                .foregroundColor(color)
        }
    }

    private func refreshValidationMessage() {
        let validation = currentValidation
        validationMessage = validation.message
        validationIsValid = validation.valid
        pointStatus = validation.pointStatus
    }

}
