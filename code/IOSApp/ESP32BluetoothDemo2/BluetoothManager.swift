import Foundation
import CoreBluetooth
import Combine
import UserNotifications

class BluetoothManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    
    @Published var connectedPeripheral: CBPeripheral?
    @Published var connectedPeripheralName: String?
    @Published var receivedData: String = ""
    
    private var centralManager: CBCentralManager!
    private let targetPeripheralName = "TipUp_Alert"
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    // MARK: - CBCentralManagerDelegate
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            startScan()
            print("Bluetooth powered on. Scanning...")
        } else {
            print("Bluetooth not available: \(central.state.rawValue)")
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String : Any], rssi RSSI: NSNumber) {
        
        let localName = advertisementData[CBAdvertisementDataLocalNameKey] as? String
        
        guard localName == targetPeripheralName else {
            print("Ignoring \(localName ?? peripheral.identifier.uuidString)")
            return
        }
        
        print("Found TipUp_Alert! Connecting...")
        connectedPeripheral = peripheral
        connectedPeripheralName = localName
        centralManager.stopScan()
        centralManager.connect(peripheral)
        peripheral.delegate = self
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("Connected to \(connectedPeripheralName ?? "Unknown")")
        peripheral.discoverServices(nil)
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        print("Disconnected from \(connectedPeripheralName ?? "Unknown")")
        connectedPeripheral = nil
        connectedPeripheralName = nil
        startScan()
    }
    
    // MARK: - CBPeripheralDelegate
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else { return }
        for service in services {
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else { return }
        for characteristic in characteristics {
            if characteristic.properties.contains(.notify) {
                peripheral.setNotifyValue(true, for: characteristic)
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard let data = characteristic.value else { return }
        receivedData = String(decoding: data, as: UTF8.self)
        print("Received: \(receivedData)")
        
        // Send local notification
        NotificationManager.shared.sendTipUpAlertNotification(message: receivedData)
    }
    
    // MARK: - Scanning
    
    func restartScan() {
        print("Restarting scan...")
        receivedData = ""
        connectedPeripheral = nil
        connectedPeripheralName = nil
        centralManager.stopScan()
        startScan()
    }
    
    private func startScan() {
        centralManager.scanForPeripherals(withServices: nil)
    }
}
