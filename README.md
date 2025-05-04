# 📘 Integrated Biometric Attendance System with IoT Capabilities and Tamper-Proof Security
[![License: CC BY-NC 4.0](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc/4.0/)

A robust and scalable biometric attendance system designed for educational institutions. This project integrates biometric authentication, IoT data access, and strong security features to deliver a reliable, portable, and user-friendly attendance tracking solution.

---

## 🚀 Features

- **👥 High Biometric Capacity**  
  Supports **up to 1000 students**, making it ideal for medium to large institutions.

- **📚 Multi-Course Support**  
  Handles **attendance for up to 20 courses**. Easily expandable via simple code modifications.

- **🔒 Advanced Security**
  - **Secure Mode**: Prevents unauthorized access and system misuse.
  - **Tamper-Proof Hardware**: Detects and resists physical tampering to maintain data integrity.

- **💾 Efficient Data Management**
  - **Local Storage**: Attendance records are saved to an onboard **SD card**.
  - **Remote Access**: Attendance sheets can be downloaded via **HTTP requests**.

- **🔋 Portability and Design**
  - Housed in a **custom 3D-printed enclosure**.
  - Powered by a **rechargeable LiPo battery** with built-in charging for true portability.

- **📲 Bluetooth Integration**
  - Streamlined **student enrollment** and **data management** through Bluetooth-enabled devices.

---

## 🛠️ Tech Stack

- **Hardware**: Fingerprint Sensor, ESP32, SD Card Module, LiPo Battery, Charging Circuit  
- **Connectivity**: Wi-Fi (HTTP), Bluetooth  
- **Storage**: SD Card (CSV/JSON format)  
- **Enclosure**: Custom 3D-Printed Case

---

## 📦 How It Works

1. **Student scans fingerprint** at the terminal.  
2. The system logs attendance with a timestamp and course ID.  
3. Data is stored on the **SD card** and can be accessed remotely.  
4. Admins use Bluetooth to enroll new students or manage existing records.

---

## 📸 Gallery


A custom 3D-printed, portable biometric attendance system with OLED display and fingerprint sensor.
<table> <tr> <td align="center"><img src="https://github.com/gitteja19/Biometric-Attendance-v2.0/blob/main/IMG_20231126_030044.jpg" width="250"/><br><sub><b>Compact Rear View</b></sub></td> <td align="center"><img src="https://github.com/gitteja19/Biometric-Attendance-v2.0/blob/main/IMG_20231126_025930.jpg" width="250"/><br><sub><b>Front View with Fingerprint Sensor</b></sub></td> <td align="center"><img src="https://github.com/gitteja19/Biometric-Attendance-v2.0/blob/main/IMG_20231126_025934.jpg" width="250"/><br><sub><b>OLED Display and Sensor Mount</b></sub></td> </tr> </table>

---

## 🔐 Security Notes

- Secure mode ensures all settings and enrollment actions require authentication.  
- Tamper detection triggers alerts and logs incidents.

---

## 📈 Future Improvements

- Cloud sync and web dashboard for real-time monitoring.  
- Integration with institutional databases (e.g., ERP or LMS platforms).  
- NFC or RFID support for hybrid authentication.

---

## 👨‍🏫 Project Supervisor

**Prof. Nagaveni IITDH** – Faculty Guide & Technical Advisor

---

## 📄 License

This project is open for educational and non-commercial use. Please refer to the `LICENSE` file for more details.
