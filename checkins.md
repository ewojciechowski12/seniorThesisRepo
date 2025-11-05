## Week 1 Summary (09/22/2025)

### This week I worked on:

1. Ordered microcontrollers, accelerometer and additional accessories.
2. Setup development environment (Arduino IDE).
3. Ran some sample projects to understand Arduino syntax and how to interface with GPIO pins on the ESP32.

### This week I learned:

1. How to flash the microcontroller with code developed in the Arduino IDE
2. How to interface with GPIO pins
3. How to print data from pins to the console
4. The different Arduino libraries I can use.

### My successes this week were:

1. Running code on the microcontroller   

### The challenges I faced this week were:

1. Setting up Arduino IDE
2. Reading data from GPIO pins

---

## Week 2 Summary (09/29/2025)
### This week I worked on:

Connecting the accelerometer to the microcontroller. I successfully accessed the accelerometer’s internal registers and displayed real-time data through the serial console in the Arduino IDE.

### This week I learned:

I learned that the accelerometer stores 2 bytes of data per axis in sequential registers. I also learned how to access these registers and correctly format the bits in my code to interpret the raw data as readable numbers.

### My successes this week were:

Successfully establishing communication between the accelerometer and the microcontroller and reading live sensor data.

### The challenges I faced this week were:

Accessing the data from the accelerometer’s registers was initially difficult since I hadn’t worked with these technologies before. After studying the accelerometer’s documentation and the I²C library, I was able to understand the communication process and extract the correct data.

---

## Week 3 Summary (10/06/2025)
### This week I worked on:

Transmitting sensor data from one microcontroller to another. I successfully connected two ESP32s using the ESP-NOW library, which enables communication within a network of ESP32 devices.

### This week I learned:

I learned how to use the ESP-NOW library, set up a communication channel, and send and receive data packets between devices.

### My successes this week were:

Successfully transmitting sensor data from the node to the central hub.

### The challenges I faced this week were:

One major challenge was learning how to use the ESP-NOW library. I also struggled to get the devices to recognize each other at first. To resolve this, I hardcoded the MAC addresses so they could send and receive packets. In the future, I hope to dynamically detect nearby ESP32 devices and automatically establish communication with them.

---

## Week 4 Summary (10/13/2025)
### This week I worked on:

Setting up Xcode and getting familiar with the overall layout and workflow for iOS development. I also began exploring Swift syntax and structure so I can eventually write an app that connects to my ESP32 to recieve notifications.

### This week I learned:

I learned how Swift syntax compares to C++—specifically around classes, functions, optionals, and property wrappers like @Published. I also learned how Xcode organizes projects, handles debugging, and interfaces with frameworks like CoreBluetooth, which I plan to use to communicate with the ESP32 over Bluetooth.

### My successes this week were:

I was able to set up Xcode and begin understanding the overall workflow for iOS development. I also made progress in grasping the fundamentals of Swift, which helped me better understand some example Bluetooth code I reviewed.

### The challenges I faced this week were:

Getting familiar with Xcode was some what more challenging than I expected. The interface was unfamiliar and I ran into issues downloading and setting up the environment. I also found Swift's syntax and structure a bit confusing compared to other compiled languages like C++, so progress was slower than I hoped this week.

---

## Week 5 Summary (10/20/2025)
### This week I worked on:

Building an IOS app that can detect an ESP32 node and send a notification to my phone once connected. Currently, the app sends a notification every 10 seconds. This required working with  Apple's CoreBluetooth framework to handle device discovery and connection events.

### This week I learned:

I learned how to detect and connect to a bluetooth device through an IOS app. From there, I learned how to send a notification to the IOS device from the app. 

### My successes this week were:

I successfully created a working IOS app that detects the ESP32 and sends notifications to my phone consistently every 10 seconds. This was a major milestone in my overall goal to send an alert to my phone from a microcontroller.

### The challenges I faced this week were:

One of the main challenges was getting the IOS app to detect bluetooth devices. This was resolved by requesting for the app to access the devices bluetooth. Without requestiong permission, the app has no access to the devices peripherals.

---

## Week 6 Summary (10/27/2025)
### This week I worked on:

This week I focused on cleaning up the code and getting it centralized into this GitHub repo. I reviewed and cleaned up the code before committing so it is easy to maintain moving forward. I also spent time working on project demo to present during my Sprint 1 reflection.

### This week I learned:

I learned how to better structure a multi-part project on GitHub. This included creating specific code folder to hold all of the source code while keeping the repository organized. 

### My successes this week were:

Successfully centralizing all project code and creating a more organized project workflow. I was also able to present a clear demo the functionality I accomplished so far.

### The challenges I faced this week were:

Not much direct development was done this week, so progress was slower compared to pervious weeks. The main challenge was cleaning and organizing the repository. I will continue to do this week after week in aims of maintaining a clean, organized project.

---

## Week 7 Summary (MM/DD/YYYY)
### This week I worked on:

[Your answer here]

### This week I learned:

[Your answer here]

### My successes this week were:

[Your answer here]

### The challenges I faced this week were:

[Your answer here]

---

## Week 8 Summary (MM/DD/YYYY)
### This week I worked on:

[Your answer here]

### This week I learned:

[Your answer here]

### My successes this week were:

[Your answer here]

### The challenges I faced this week were:

[Your answer here]

---

## Week 9 Summary (MM/DD/YYYY)
### This week I worked on:

[Your answer here]

### This week I learned:

[Your answer here]

### My successes this week were:

[Your answer here]

### The challenges I faced this week were:

[Your answer here]

---

## Week 10 Summary (MM/DD/YYYY)
### This week I worked on:

[Your answer here]

### This week I learned:

[Your answer here]

### My successes this week were:

[Your answer here]

### The challenges I faced this week were:

[Your answer here]

---

## Week 11 Summary (MM/DD/YYYY)
### This week I worked on:

[Your answer here]

### This week I learned:

[Your answer here]

### My successes this week were:

[Your answer here]

### The challenges I faced this week were:

[Your answer here]

---

## Week 12 Summary (MM/DD/YYYY)
### This week I worked on:

[Your answer here]

### This week I learned:

[Your answer here]

### My successes this week were:

[Your answer here]

### The challenges I faced this week were:

[Your answer here]

---

## Week 13 Summary (MM/DD/YYYY)
### This week I worked on:

[Your answer here]

### This week I learned:

[Your answer here]

### My successes this week were:

[Your answer here]

### The challenges I faced this week were:

[Your answer here]

---
