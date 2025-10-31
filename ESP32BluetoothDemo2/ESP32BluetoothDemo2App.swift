//
//  ESP32BluetoothDemoApp.swift
//  ESP32BluetoothDemo
//
//  Created by Ethan W on 10/15/25.
//

import SwiftUI
import UserNotifications

@main
struct ESP32BluetoothDemo2App: App {
    init() {
        // Request notification permissions at app launch
        NotificationManager.shared.requestPermission()
        
        // Set notification delegate to show notifications while app is foreground
        UNUserNotificationCenter.current().delegate = NotificationManager.shared
    }
    
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}
