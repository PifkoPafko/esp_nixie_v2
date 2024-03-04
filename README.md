| Supported Target | ESP32-S3 |
| ---------------- | -------- |

# Remotely synchronized 16-NIXIE clock with Wi-Fi and Bluetooth LE

This is ESP-IDF project for ESP32-S3 which drives device named NIXIE Clock.

## Features

- Displaying time and date
- Wi-Fi time and date auto-synchronization
- Adding, modifying, deleteing and enabling alarms via [application](https://github.com/PifkoPafko/android_nixie_v2) or via 3 manual buttons.
- Playing alarms with WAVE file
- Maintaining correct time and date with RTC module
- Manual time and date correction

### Hardware Required

* ESP32-DevKitC
* 16 x IN-14 NIXIE Tubes
* RTC DS3231 module
* Waveshare 3947 module
* MAX98357A module
* 190 V DC power supply for NIXIE Tubes
* Power supply for other elements

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

### Photos

![image](https://github.com/PifkoPafko/esp_nixie_v2/assets/65284616/480d9a35-001f-4f64-b54f-bba5786677c8)
![image](https://github.com/PifkoPafko/esp_nixie_v2/assets/65284616/bcc214e8-79e6-447a-b77f-987b2378325f)
