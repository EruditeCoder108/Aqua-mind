<p align="center">
  <img src="https://img.shields.io/badge/💧-Aqua--Mind-00d4ff?style=for-the-badge&labelColor=0a1628" alt="Aqua-Mind"/>
</p>

<h1 align="center">Aqua-Mind</h1>
<h3 align="center">Context-Aware Water Quality Intelligence</h3>

<p align="center">
  <strong>Jal Jeevan Mission Innovation Challenge | Portable Water Quality Devices Track</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Python-3.9+-blue?style=flat-square&logo=python" alt="Python"/>
  <img src="https://img.shields.io/badge/Raspberry_Pi-Zero_2_W-c51a4a?style=flat-square&logo=raspberrypi" alt="Raspberry Pi"/>
  <img src="https://img.shields.io/badge/Gemini-AI_Powered-4285F4?style=flat-square&logo=google" alt="Gemini"/>
  <img src="https://img.shields.io/badge/PWA-Offline_First-5A0FC8?style=flat-square" alt="PWA"/>
  <img src="https://img.shields.io/badge/Cost-₹3,350-green?style=flat-square" alt="Cost"/>
</p>

---

## 🌊 The Problem

Lab tests are **accurate but slow**. Cheap sensors are **fast but lie**.

Previous student projects failed because they tried to compete on hardware precision—which costs money.

## 💡 Our Solution

**Aqua-Mind competes on Software Intelligence, not hardware precision.**

We use a **5-Stage Trust Architecture** to validate cheap sensor data before showing it to users. The result: lab-grade *confidence* at a fraction of the cost.

---

## 🏗️ Architecture

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   SENSORS   │───▶│  TRUST      │───▶│   MOBILE    │
│  TDS/Turb   │    │  ENGINE     │    │   APP       │
│  Temp       │    │  (5 Pillar) │    │   (PWA)     │
└─────────────┘    └─────────────┘    └─────────────┘
      │                   │                  │
      ▼                   ▼                  ▼
   ₹1,300             FREE              Gemini AI
   Hardware          Software           "Doctor"
```

---

## 🛡️ The 5 Pillars of Trust

| Pillar | What It Does | Why It Matters |
|--------|--------------|----------------|
| **1. Statistical Tri-Check** | 3 bursts × 5 samples each | Filters sensor noise |
| **2. Stability Index** | Variance analysis over time | Detects probe fouling |
| **3. Geo-Adaptive Profiling** | Region-specific thresholds | Jabalpur ≠ Jaipur |
| **4. Offline Rule Engine** | BIS IS:10500 safety rules | Works without internet |
| **5. AI Chemist (Gemini)** | "Doctor-style" explanations | Hindi/English advice |

---

## 📱 Features

### Python Backend
- ✅ Simulation mode for testing without hardware
- ✅ 6 regional profiles (Jabalpur, Jaipur, Chennai, Delhi, Guwahati, Mumbai)
- ✅ Seasonal adjustments (Monsoon vs Dry season)
- ✅ Bluetooth communication with mobile app
- ✅ CLI interface for standalone operation

### Mobile App (PWA)
- ✅ Real-time sensor readings with animated Jal-Score
- ✅ Web Bluetooth API for device connection
- ✅ Gemini AI integration for intelligent analysis
- ✅ Offline-first with service workers
- ✅ Beautiful dark ocean theme

---

## 🚀 Quick Start

### Test Without Hardware

```bash
# Clone the repository
git clone https://github.com/EruditeCoder108/Aqua-mind.git
cd Aqua-mind/pi

# Run with simulation
python main.py --scenario tap_water --single

# Try different water conditions
python main.py --scenario dirty_water --single
python main.py --scenario contaminated --single

# Interactive mode
python main.py --scenario tap_water
```

### Test Mobile App

1. Open `mobile-app/index.html` in Chrome
2. Click ⚙️ Settings → Set Simulation Mode to "Tap Water"
3. Click Connect → Watch the magic! ✨

---

## 🔧 Hardware Setup

### Bill of Materials (~₹3,350)

| Component | Specification | Price (₹) |
|-----------|---------------|-----------|
| Raspberry Pi Zero 2 W | Quad-core, WiFi+BT | 1,500-1,800 |
| MicroSD Card | 16GB Class 10 | 350 |
| TDS Sensor | Analog, 0-1000ppm | 350 |
| Turbidity Sensor | Analog, 0-5V | 850 |
| DS18B20 | Waterproof temp probe | 100 |
| MCP3008 ADC | 8-ch 10-bit SPI | 150 |
| Misc (resistors, wires) | - | 150 |

### Wiring Diagram

```
                    RASPBERRY PI ZERO 2 W
                    ┌─────────────────────┐
    TDS Sensor ────▶│ MCP3008 CH0        │
    Turbidity* ────▶│ MCP3008 CH1        │
    DS18B20 ───────▶│ GPIO 4             │
    Button ────────▶│ GPIO 17            │
                    └─────────────────────┘

    * CRITICAL: Use voltage divider (2× 10kΩ) for Turbidity!
      Turbidity outputs 4.5V - will damage Pi without divider.
```

### Raspberry Pi Setup

```bash
# Enable SPI
sudo raspi-config  # Interface Options → SPI → Enable

# Install dependencies
sudo apt update && sudo apt install -y python3-pip python3-numpy
pip3 install spidev RPi.GPIO

# Run
cd ~/Aqua-mind/pi
python3 main.py --profile JABALPUR
```

---

## 📁 Project Structure

```
Aqua-mind/
├── pi/                         # Raspberry Pi backend
│   ├── main.py                 # Main orchestrator
│   ├── sensors.py              # Hardware drivers + simulation
│   ├── trust_engine.py         # 5-Pillar Trust System
│   ├── rules_engine.py         # Offline safety rules
│   ├── bluetooth_comm.py       # Bluetooth serial
│   ├── profiles.json           # Regional configurations
│   └── calibration.json        # Sensor calibration data
│
└── mobile-app/                 # Progressive Web App
    ├── index.html              # Main dashboard
    ├── css/style.css           # Dark ocean theme
    ├── js/
    │   ├── app.js              # Main app logic
    │   ├── bluetooth.js        # Web Bluetooth API
    │   └── gemini.js           # AI integration
    ├── manifest.json           # PWA manifest
    └── sw.js                   # Service worker
```

---

## 🌍 Regional Profiles

| Region | Primary Concern | TDS Weight | Turbidity Weight |
|--------|-----------------|------------|------------------|
| **Jabalpur, MP** | Monsoon sediment | 30% | 60% |
| **Jaipur, RJ** | High TDS, Arsenic | 70% | 20% |
| **Chennai, TN** | Coastal salinity | 60% | 30% |
| **Delhi NCR** | Organic pollution | 40% | 50% |
| **Guwahati, AS** | Flood contamination | 20% | 70% |
| **Mumbai, MH** | Industrial effluent | 50% | 40% |

---

## 🤖 AI Integration

Aqua-Mind uses Google's Gemini API to provide "Doctor-style" analysis:

```json
{
  "status": "Caution",
  "headline": "पानी में TDS ज़्यादा है",
  "explanation": "Your water has 650ppm dissolved solids...",
  "action": "Use RO filter or mix with low-TDS water"
}
```

### Get Your API Key
1. Visit [Google AI Studio](https://aistudio.google.com/apikey)
2. Create a new API key
3. Add it in the mobile app settings

---

## 🎬 Demo Script (2 Minutes)

**[0:00-0:30] THE HOOK**
> "Lab tests are accurate but slow. Cheap sensors are fast but lie. I present Aqua-Mind, the only low-cost device that knows when it's wrong."

**[0:30-1:00] THE DEMO**
> "Watch this. I dip it in dirty water. It doesn't just give a number. It runs a 3-burst statistical check."

**[1:00-1:30] THE AI MAGIC**
> "Now, I check my phone. Gemini tells me in Hindi: 'पानी गंदा है, उबाल कर पीएं'"

**[1:30-2:00] CLOSING**
> "We packaged the wisdom of a lab scientist into a ₹3,000 device. This is how we solve trust in the Jal Jeevan Mission."

---

## 📜 Standards Compliance

- **BIS IS:10500:2012** - Indian Standard for Drinking Water
- **WHO Guidelines** for Drinking Water Quality
- **Jal Jeevan Mission** specifications

---

## 🛣️ Roadmap

- [x] Core Python backend with simulation
- [x] 5-Pillar Trust System
- [x] Mobile PWA with Gemini AI
- [x] Regional profiles (6 cities)
- [ ] Hardware integration testing
- [ ] Sensor calibration interface
- [ ] Data export (CSV/PDF reports)
- [ ] Multi-language support

---

## 👨‍💻 Author

**Jal Jeevan Mission Innovation Challenge Entry**  
📍 Jabalpur, Madhya Pradesh

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

---

<p align="center">
  <strong>🌊 Clean Water for All | जल जीवन मिशन 🇮🇳</strong>
</p>
