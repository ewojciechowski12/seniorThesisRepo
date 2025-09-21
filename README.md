# Senior Thesis Repo: ESP32 Driven Tip-Up Alert System

## Software Requirements Specification for the ESP32 driven alert system

## Introduction

### Purpose
The purpose of this document is to define the functional and non-functional requirements for a netowork of ESP32 microcontrollers that form an ice fishing tip-up alert system.

The key goals are:
- Detect tip-up flag movement using an accelerometer
- Transmit sensor data from each tip-up node to a central ESP32
- Connect the central ESP32 to a cellular device via Bluetooth Low Energy (BLE)
- Deliver real-time alerts and status updates to the user's phone

### Scope
This system is intended to support ice fishing by providing a wireless alert system for multiple tip-ups within range of a central hub. The system will handle:
 - Detection of tip-up flag movement via accelerometer sensors.
 - Wireless transmission of sensor data from each node to a central ESP32 hub
 - Communication between the central ESP32 and a cellular device via BLE
 - Deliver real time alerts and status updates to the user's phone
 - Low-power operation to ensure sufficient battery life for extended use in extreme outdoor conditions

The scope of this project can also include tools for the user to:
- View the current status of all connected tip-ups from a mobile device
- Reset the state of a tip-up from "tripped" back to "set" via the mobile device
- Monitor connectivity and battery status of each tip-up node.

### Definitions, Acronyms, and Abbreviations
- **ESP32**: a low-cost, low-power microcontroller with built in Wi-Fi and Bluetooth capabilities, used as the processing and communication unit in this system.
- **Tip-Up (node)**: An ice-fishing device equipped with a flag that is released when a fish takes the bait. In this system, each tip-up is paired with an ESP32-based sensor node.
- **BLE (Bluetooth Low Energ)**: A wireless communication protocol optimized for short-range, low-power data transmission. It will be used to send alerts from the ESP32 hub to the user's mobile device.


## Overview

The ESP32 Tip-Up Alert System is a wireless monitoring platform designed to automate the detection and notification of ice fishing tip-up activity. It serves as the primary interface for anglers to track the status of their fishing lines and ensures timely alerts when a flag is triggered, reduing the chance of missed catches.

### System Features:
1. **Motion Detection**: Each tip-up node is equipped with an accelerometer to detect when the flag moves into the "up" position, indicating a fish strike
2. **Wirelsss Communication**: Sensor data is transmitted from each tip-up node to a central ESP32 hub using Bluetooth Low Energy (BLE)
3. **Mobile Integration**: The central hub connects to a user's mobile device via BLE, providing real-time alerts and status updates through a dedicated interface
4. **Remote Reset**: Users can reset the state of a tip-up from "tripped" back to "set" directly from their phone after attending the catch.
5. **Battery Monitoring**: The system tracks the battery status of each tip-up node to ensure reliable operation during extended ice fishing seasons.

The system is designed with reliability and low power consumption in mind, allowing multiple tip-ups to operate simultaneously for long periods in outdoor conditions. It will leverage the ESP32's built-in Bluetooth capabilities to provide a simple, low-cost, and user-friendly solution for ice fishing.

The following sections detail the specific use cases the system will support, describing how anglers will interact with the tip-up nodes, central hub, and mobile device during typical fishing operations.

## Use Cases

### 1: Setting a Tip-Up
- The angler places a tip-up in the fishing hole and sets the flag mechanism.
- The tip-up node detects that it has been armed and enters monitoring mode.
- The node communicates its "set" status to the central hub.
- The central hub updates the mobile device interface to show that the tip-up is active.

**2. Dtecting a Fish Strike**:
- A fish pulls the line, releasing the flag into the "up" position.
- The accelerometer detects movement and triggers an interrupt on the ESP32 node.
- The node updates its state to "tripped" and transmits an alert to the central hub.
- The central hub relays this event to the angler's mobile device, which issues a sound, vibration, or visual alert.

**3 Resetting a Tip-Up**
- After retrieving the fish, the angler resets the tip-up flag mechanism.
- The angler confirms the reset on their mobile device.
- The mobile device sens a BLE command to the central hub, which forwards it to the corresponding tip-up node.
- The tip-up node updates its state to "set" and resumes monitoring for new triggers

**4. Monitoring Battery and Connectivity**:
- Each tip-up node periodically reports its battery status to the central hub.
- The central hub relays this information to the mobile device, allowing the angler to view battery levels for all active tip-ups.
- If a tip-up node loses connection, the mobile device provides a notification indicating potential range or power issues.
