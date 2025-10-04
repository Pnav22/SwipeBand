#  Swipe Band  
A wearable armband game powered by ESP32 and a flexible display

![Swipe Band Mockup](CONCEPT.jpg) <!-- Optional: replace with your image -->

##  Overview
**Swipe Band** is a lightweight wearable game system built around an **ESP32** microcontroller and a **flexible OLED/TFT display**. The game challenges players to **swipe in the direction of on-screen arrows** as quickly and accurately as possible. This is inspired by the HellDivers 2 Strategem call downs. To call down a strategem in Hell Divers the player must enter a combination of arrow keys which then would activate the ability. In the game the player does this on a arm mounted screen. 

---

## Features
- **ESP32-powered** — runs the entire game logic locally  
-  **Flexible or curved display** — mounted on an armband for comfort and style  
- **Swipe detection** via capacitive touch panel or IMU-based gestures  
- **Battery-powered** for portable gameplay.
- **Interchangable size** - the band uses velcro strips that can be changed to fit any arm size. 

---

##  Hardware Components
| Component                        | Description                                     |
|-----------------------------------|-------------------------------------------------|
| ESP32-S3 / ESP32 Dev Board      | Main microcontroller (Wi-Fi + BLE capable)     |
| Flexible OLED or TFT display    | SPI-based, 1.3"–2.0" recommended               |
| FT6206 Capacitive Touch Panel  | Detects swipes and taps                        |
| Li-Po Battery (500–1000 mAh)   | Portable power supply                          |
| TP4056 Charging Module        | For battery charging via USB                   |
|Black Filament| To print out case|
|Sand Paper| To post proccess print|

---
