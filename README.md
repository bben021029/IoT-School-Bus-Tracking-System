# 🚌 IoT School Bus Tracking System

An IoT-based school bus monitoring solution using GPS and RFID technology to track real-time location and student attendance, aiming to improve safety and provide visibility into school transportation.

## 🧾 Overview

This system is developed to track the location of a school bus in real time and log students’ boarding and alighting activities using RFID. Parents and administrators can view this data through a web interface. The solution is built with an Uno-ESP32 microcontroller and communicates with ThingSpeak for cloud-based data visualization.

## ✨ Features

- **Live Bus Location Tracking** using the Neo-6M GPS module  
- **RFID-based Student Logging** with PN532 reader  
- **LCD Display** for status messages and scanned student information  
- **Data Upload to ThingSpeak** for cloud visualization  
- **Real-time Route Mapping** with Leaflet.js and OpenRouteService  
- **No GSM Module Used** – Communication is over WiFi via Uno-ESP-32

## 🧩 Architecture & Components

The system architecture consists of the following:

- **Uno-ESP32** as the main controller, connecting to WiFi and managing all sensors  
- **PN532 RFID Reader** to detect student IDs during boarding/alighting  
- **Neo-6M GPS Module** to get latitude and longitude coordinates continuously  
- **I2C LCD Display** to show current system status and student names/IDs  
- **ThingSpeak** as the cloud backend for storing and plotting data  
- **Web Dashboard** built with HTML, JavaScript, Leaflet.js, and OpenRouteService for map rendering and route visualization

## 🔧 Hardware Requirements

- Uno-ESP32  
- PN532 RFID Module  
- Neo-6M GPS Module  
- I2C LCD Display (e.g., 16x2 or 20x4)  
- Breadboard, jumper wires  

## 💻 Software Requirements

- Arduino IDE (with necessary board and library installations)  
- Required Arduino libraries:  
  - Wire.h
  - WiFi.h
  - ThingSpeak.h
  - WiFiUdp.h
  - NTPClient.h
  - HTTPClient.h
  - TinyGPS++.h
  - Adafruit_PN532.h
  - LiquidCrystal_I2C.h
- ThingSpeak account with public or private channel  
- HTML/JS web interface to visualize data and routes

## 🚀 Getting Started

1. **Connect the hardware**:
   - Wire the GPS to the correct UART pins on the Uno-ESP32
   - Connect the PN532 using I2C
   - Connect the LCD to I2C pins (SDA/SCL)
   
2. **Configure WiFi and ThingSpeak credentials** in the Arduino sketch

3. **Upload the Arduino code** to the Uno-ESP32 using Arduino IDE

4. **Launch the web dashboard** to visualize the bus location and student log

## 📘 Usage

- Students tap their RFID cards when boarding or alighting  
- The system logs the timestamp, coordinates, and student ID to ThingSpeak  
- The LCD screen displays the student ID and confirmation  
- The web interface displays:
  - Live bus position
  - Trip direction (home → school or school → home)
  - Recent RFID logs

## 🔮 Future Improvements

- Add support for push notifications or integration with messaging APIs  
- Implement geofencing alerts for out-of-bound routes  
- Add secure login and data encryption for web dashboard  
- Support for historical route replay and attendance summary reports  
