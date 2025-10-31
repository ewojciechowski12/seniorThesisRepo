import SwiftUI

struct ContentView: View {
    
    @StateObject private var bluetoothManager = BluetoothManager()
    
    var body: some View {
        VStack(spacing: 20) {
            Text("ESP32 Tip-Up Alert")
                .font(.title)
                .bold()
            
            // Connection status
            if let name = bluetoothManager.connectedPeripheralName {
                Text("Connected to \(name)")
                    .foregroundColor(.green)
            } else {
                Text("Scanning for TipUp_Alert...")
                    .foregroundColor(.orange)
            }
            
            // Received data display
            ScrollView {
                Text(bluetoothManager.receivedData.isEmpty ? "Waiting for tip-up alert..." : bluetoothManager.receivedData)
                    .font(.system(.body, design: .monospaced))
                    .padding()
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .background(Color(.systemGray6))
                    .cornerRadius(12)
            }
            .frame(height: 200)
            
            // Manual restart scan
            Button(action: {
                bluetoothManager.restartScan()
            }) {
                Label("Restart Scan", systemImage: "arrow.clockwise")
            }
            .buttonStyle(.borderedProminent)
        }
        .padding()
    }
}
