# 🌊 Aqua-Mind: Water Quality Intelligence

**Lab-grade water testing for ₹1,500 using AI + Smart Sensors**

Built for the Jal Jeevan Mission. Designed for every Indian village.

---

## 📦 What You Need

| Item | Cost (approx) | Status |
|------|---------------|--------|
| ESP32 DevKit (30-pin) | ₹300 | ✅ Required |
| TDS Sensor V1.0 | ₹200 | ✅ Required |
| pH Sensor Module | ₹400 | ✅ Required |
| Turbidity Sensor | ₹400 | ⏳ Coming soon |
| Jumper wires | ₹50 | ✅ Required |
| USB Cable (Micro-USB) | ₹50 | ✅ Required |

**Total: ~₹1,400** (compared to ₹50,000 lab equipment!)

---

## 🔌 Wiring Diagram

Connect your sensors to ESP32 like this:

```
ESP32 Pin    →    Sensor
──────────────────────────────────────
3.3V         →    pH Module VCC (red)
GND          →    pH Module GND (black)
GPIO 35      →    pH Module Signal (yellow/blue)

VIN (5V)     →    TDS Module VCC (red)
GND          →    TDS Module GND (black)
GPIO 34      →    TDS Module Signal (yellow/A)
──────────────────────────────────────
```

### Visual Guide:
```
        ESP32 Board
    ┌─────────────────┐
    │  [USB Port]     │
    │                 │
    │ 3V3 ─────→ pH VCC
    │ GND ─────→ pH GND
    │ G35 ─────→ pH Signal
    │                 │
    │ VIN ─────→ TDS VCC
    │ GND ─────→ TDS GND
    │ G34 ─────→ TDS Signal
    │                 │
    └─────────────────┘
```

---

## 🛠️ Step-by-Step Setup Guide

### Step 1: Download Arduino IDE

1. Go to: https://www.arduino.cc/en/software
2. Click **"Windows Win 10 and newer, 64 bits"**
3. Download and install (click Next, Next, Install)
4. Open Arduino IDE

### Step 2: Add ESP32 Support to Arduino

1. In Arduino IDE, go to **File → Preferences**
2. Find "Additional boards manager URLs" and paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Click **OK**
4. Go to **Tools → Board → Boards Manager**
5. Search "ESP32"
6. Click **Install** on "ESP32 by Espressif Systems"
7. Wait for installation (takes 2-3 minutes)

### Step 3: Select Your Board

1. Connect ESP32 to computer via USB cable
2. Go to **Tools → Board → ESP32 Arduino**
3. Select **"ESP32 Dev Module"**
4. Go to **Tools → Port**
5. Select the COM port (usually COM3, COM4, or similar)

### Step 4: Upload the Code

1. Open the file `aquamind_esp32.ino` in Arduino IDE
2. Click the **Upload** button (→ arrow icon)
3. **Important:** Hold the **BOOT** button on ESP32 when you see "Connecting..."
4. Release when upload starts
5. Wait for "Done uploading"

### Step 5: Test It!

1. Go to **Tools → Serial Monitor**
2. Set baud rate to **115200** (bottom right)
3. Press the **BOOT** button on ESP32
4. Watch the water analysis run!

---

## 📱 Connecting to Mobile App

1. Open the Aqua-Mind app on your phone
2. Turn on Bluetooth
3. Look for device named **"AquaMind-ESP32"**
4. Tap to connect
5. Tap "Analyze" to test water

---

## 🧪 How to Test Water

1. **Dip sensors** in water sample
2. **Press BOOT button** on ESP32 (or use app)
3. **Wait 5 seconds** for analysis
4. **Read result** on Serial Monitor or app

### Understanding Results

| Jal-Score | Verdict | What it means |
|-----------|---------|---------------|
| 80-100 | ✅ SAFE | Good to drink |
| 60-79 | 👍 ACCEPTABLE | OK, could be better |
| 40-59 | ⚠️ CAUTION | Filter before drinking |
| 0-39 | 🚫 UNSAFE | Do not drink! |

---

## 🔧 Troubleshooting

### "Upload Failed" Error
- Hold BOOT button while uploading
- Try a different USB cable
- Check Tools → Port is selected

### Sensor Reads 0
- Check wiring connections
- Make sure sensor is in water
- Try swapping GPIO pins

### BLE Not Found
- Restart ESP32 (press EN button)
- Turn phone Bluetooth off and on
- Try forgetting and re-pairing device

---

## 📁 Project Structure

```
aqua-mind/
├── aquamind_esp32.ino   # Main code for ESP32
├── mobile-app/          # Android app source code
├── README.md            # This file
├── LICENSE              # MIT License
└── IJRPR31819.pdf       # Research paper reference
```

---

## 🎯 What the Code Does

1. **Reads TDS** (Total Dissolved Solids) - Measures minerals in water
2. **Reads pH** - Measures acidity/alkalinity (7 = neutral)
3. **Tri-Check** - Takes 15 readings to ensure accuracy
4. **Calculates Jal-Score** - Combines all readings into 0-100 score
5. **Sends via Bluetooth** - Displays result on your phone

---

## ⚡ Quick Reference

| Action | How to do it |
|--------|--------------|
| Upload code | Click → button, hold BOOT on ESP32 |
| Test water | Press BOOT button on ESP32 |
| View results | Serial Monitor at 115200 baud |
| Connect phone | Bluetooth → AquaMind-ESP32 |

---

## 🤝 Credits

- **Author:** Sambhav Jain
- **Project:** Jal Jeevan Mission Innovation Challenge
- **Location:** Dhanwantri Nagar, Jabalpur, MP
- **License:** MIT (Open Source)

---

## 📞 Need Help?

- Check the Troubleshooting section above
- Open an issue on GitHub
- Email: eruditecoder108@gmail.com
