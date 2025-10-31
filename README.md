# Senior Thesis Repo: ESP32 Driven Tip-Up Alert System - MVP

## Software Requirements Specification for the ESP32 Tip-Up Alert System - MVP

## Introduction

### Purpose
The purpose of this document is to define the MVP specifications and requirements for a network of ESP32 microcontrollers that form an ice fishing tip-up alert system.
This document will highlight the core functionalities needed for the system, disregarding full implementation specifications.

The key goals are:
- Connect accelerometer to ESP32 microcontroller
- Transmit sensor data from ESP32 node to another ESP32 that will act as central hub
- Develop a way for sensor data to be viewed/accessed via a mobile device

### Scope
This system is intended to support ice fishing by providing a wireless alert system for multiple tip-ups within range of a central hub. The system will handle:
 - movement detection through an accelerometer
 - Wireless transmission of sensor data from sensor driven ESP32 to a central ESP32 hub
 - Dashboard of sensor data to be viewed by a mobile device

### Definitions, Acronyms, and Abbreviations
- **ESP32**: a low-cost, low-power microcontroller with built in Wi-Fi and Bluetooth capabilities, used as the processing and communication unit in this system

## Overview

The ESP32 Tip-Up Alert System is a wireless monitoring platform designed to automate the detection and notification of ice fishing tip-up activity. It serves as the primary interface for anglers to track the status of their fishing lines and ensures timely alerts when a flag is triggered, reduing the chance of missed catches.

### System Features:
1. **Motion Detection**: ESP32 node will detect motion through an accelerometer 
2. **Wirelsss Communication**: Sensor data is transmitted from each node to a central ESP32 hub using Bluetooth Low Energy (BLE)
3. **Mobile Integration**: The central hub will host a small web-server that will host an access point that can be connected to by a mobile device to view sensor data

## Use Cases

### 1: Motion Detected
- Motion is detected from the device through an accelerometer
- Data is then relayed to central hub from node

**2. ESP32 web server**:
- A web server will be hosted on the central hub
- Web server will have an access point, able to be connected to via mobile device

**3. View Sensor Data**
- On the web server, a simple web page will be hosted that displays sensor data from node

