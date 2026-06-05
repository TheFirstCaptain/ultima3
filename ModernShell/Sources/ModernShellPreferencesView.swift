import SwiftUI

struct ModernShellPreferencesView: View {
    @State private var showTileGrid = true
    @State private var soundEnabled = true
    @State private var scale = 2.0

    var body: some View {
        Form {
            Toggle("Tile grid", isOn: $showTileGrid)
            Toggle("Sound", isOn: $soundEnabled)
            HStack {
                Text("Scale")
                Slider(value: $scale, in: 1...4, step: 1)
                Text("\(Int(scale))x")
                    .monospacedDigit()
            }
        }
        .padding(20)
        .frame(minWidth: 360, minHeight: 220)
    }
}
