//
//  TipUpAlert.swift
//  ESP32BluetoothDemo2
//
//  Created by Ethan W on 10/21/25.
//

import SwiftUI
import UserNotifications


struct TipUpAlert: App {
    
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
