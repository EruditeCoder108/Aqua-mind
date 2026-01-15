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

## 💡 Our Solution

**Aqua-Mind competes on Software Intelligence, not hardware precision.**

We use a **5-Stage Trust Architecture** to validate cheap sensor data before showing it to users.

**Tagline**: *"Lab-aligned confidence at ₹3,000"*

---

## 🚀 Quick Start (No Hardware Needed!)

```bash
# Clone
git clone https://github.com/EruditeCoder108/Aqua-mind.git
cd Aqua-mind/pi

# Test different water scenarios
python main.py --scenario clean_water --single    # Good water
python main.py --scenario tap_water --single      # Normal tap  
python main.py --scenario dirty_water --single    # Bad water
python main.py --scenario contaminated --single   # Unsafe
python main.py --scenario sensor_error --single   # Broken sensor
```

### Test Mobile App
1. Open `mobile-app/index.html` in Chrome
2. Settings → Set Simulation Mode → "Tap Water"
3. Click Connect → Watch the magic! ✨

### Test Ghost Map
```
Open: mobile-app/map.html
```

---

## 📖 How It Works (Simple Explanation)

```
CHEAP SENSOR (₹300)  →  SMART SOFTWARE (FREE)  →  RELIABLE RESULTS
       ↓                        ↓                        ↓
   "350 ppm"              "Is this real?"           "CAUTION ⚠️"
   (might be wrong)       (5 trust checks)          (with confidence)
```

---

## 🐍 Part 1: Python Backend (The Brain)

### `sensors.py` - The Eyes 👀
Reads raw numbers from sensors. Has **SIMULATION MODE** for testing without hardware.

### `trust_engine.py` - The Detective 🔍
The **HEART** of the project. Uses **5 tricks** to catch lying sensors:

| Trick | What It Does |
|-------|--------------|
| **Tri-Check** | Take 3 readings, average them (not just 1) |
| **Stability Score** | 0-100% reliability rating |
| **Geo-Profile** | Different rules for Jabalpur vs Jaipur vs Chennai |
| **Seasonal Awareness** | Monsoon = expect more sediment |
| **Jal-Score** | Final 0-100 water quality score |

### `rules_engine.py` - The Offline Doctor 💊
Gives safety advice **WITHOUT internet** (important for villages!)

### `bluetooth_comm.py` - The Messenger 📡
Sends results from Pi to your phone via Bluetooth.

### `profiles.json` - The Map 🗺️
Stores settings for different cities (Dhanwantri Nagar, Jabalpur, Jaipur, Chennai, Delhi, Guwahati, Mumbai).

### `main.py` - The Boss 👔
Connects everything together.

---

## 📱 Part 2: Mobile App (The Face)

| File | What It Does |
|------|--------------|
| `index.html` | Main dashboard with Jal-Score |
| `map.html` | 🗺️ **Ghost Map** - Network visualization |
| `style.css` | Dark ocean theme |
| `app.js` | Main app logic |
| `bluetooth.js` | Connects to Raspberry Pi |
| `gemini.js` | Talks to Google AI |
| `sw.js` | Makes app work offline |

---

## 🗺️ Ghost Map (Network Intelligence)

**The killer demo feature!** Shows how Aqua-Mind can scale to a city-wide network.

### What You See:
- 🔵 **Your Device** (Pulsing Cyan) - Live analysis
- 🟢 **Safe Neighbors** - Score 80+
- 🟠 **Caution Neighbors** - Score 50-80
- 🔴 **Unsafe Nodes** - Score <50
- 📍 **Red Polygon** - Contamination cluster detected!

### Demo Script:
> "My device is analyzing (Cyan). The system detected a **cluster of high turbidity** (Red Zone) - alerting the water department that a pipe has likely burst in Sector 4."

---

## 🤖 AI Integration (Gemini)

Takes numbers → Explains in simple Hindi/English:

**Input**: `TDS: 650, Turbidity: 3.2, Score: 58`

**Output**: 
> "पानी में TDS ज़्यादा है (High minerals detected). Use RO filter recommended."

### Get API Key:
1. Visit [Google AI Studio](https://aistudio.google.com/apikey)
2. Create new key
3. Add in mobile app settings

---

## 🔧 Hardware Setup

### Bill of Materials (~₹3,350)

| Component | Price (₹) |
|-----------|-----------|
| Raspberry Pi Zero 2 W | 1,500-1,800 |
| MicroSD Card 16GB | 350 |
| TDS Sensor | 350 |
| Turbidity Sensor | 850 |
| DS18B20 Temp Probe | 100 |
| MCP3008 ADC | 150 |
| Misc (resistors, wires) | 150 |

### Wiring
```
TDS Sensor ────▶ MCP3008 CH0
Turbidity* ────▶ MCP3008 CH1  (* Use voltage divider!)
DS18B20 ───────▶ GPIO 4
Button ────────▶ GPIO 17
```

⚠️ **CRITICAL**: Turbidity outputs 4.5V - use voltage divider (2× 10kΩ) or damage Pi!

---

## 📁 Project Structure

```
Aqua-mind/
├── pi/                         # Raspberry Pi backend
│   ├── main.py                 # The Boss
│   ├── sensors.py              # The Eyes
│   ├── trust_engine.py         # The Detective
│   ├── rules_engine.py         # The Doctor
│   ├── bluetooth_comm.py       # The Messenger
│   ├── profiles.json           # The Map
│   └── calibration.json        
│
└── mobile-app/                 # Progressive Web App
    ├── index.html              # Dashboard
    ├── map.html                # Ghost Map
    ├── css/style.css
    ├── js/app.js
    ├── js/bluetooth.js
    ├── js/gemini.js
    ├── manifest.json
    └── sw.js
```

---

## 🌍 Regional Profiles

| Region | Primary Concern | Focus |
|--------|-----------------|-------|
| **Dhanwantri Nagar, Jabalpur** | Narmada sediment | Turbidity 55% |
| **Jabalpur City** | Monsoon sediment | Turbidity 60% |
| **Jaipur** | High TDS/Arsenic | TDS 70% |
| **Chennai** | Coastal salinity | TDS 60% |
| **Delhi** | Organic pollution | Turbidity 50% |
| **Guwahati** | Floods | Turbidity 70% |
| **Mumbai** | Industrial | TDS 50% |

---

## 🎬 Demo Script (2 Minutes)

**[0:00-0:30] THE HOOK**
> "Lab tests are accurate but slow. Cheap sensors are fast but lie. Aqua-Mind is the only low-cost device that knows when it's wrong."

**[0:30-1:00] THE DEMO**
> "Watch. I dip it in dirty water. It runs a 3-burst statistical check, not just one reading."

**[1:00-1:30] THE AI + MAP**
> "My phone shows Gemini saying: 'पानी गंदा है'. And the Ghost Map shows a contamination cluster in Sector 4!"

**[1:30-2:00] CLOSING**
> "Lab-aligned confidence at ₹3,000. This is how we solve trust in the Jal Jeevan Mission."

---

## ✅ What's Done

- [x] Core Python backend with simulation
- [x] 5-Pillar Trust System (Tri-Check, Stability, Geo-Profile, Rules, AI)
- [x] Mobile PWA with Gemini AI
- [x] Ghost Map network visualization
- [x] 7 Regional profiles (research-backed)
- [x] Offline-first architecture

## 🔜 Coming Next

- [ ] Hardware integration testing
- [ ] Real sensor calibration  
- [ ] Data export (CSV/PDF)
- [ ] Multi-language UI

---

## 📜 Standards & Research

- **BIS IS:10500:2012** - Indian Drinking Water Standard
- **WHO Guidelines** for Drinking Water Quality
- **Research**: IJRPR31819 - Analysis of Drinking Water in Jabalpur City (2024)

---

## 📄 License

MIT License - Open Source

---

<p align="center">
  <strong>🌊 Clean Water for All | जल जीवन मिशन 🇮🇳</strong><br>
  <em>"Lab-aligned confidence at ₹3,000"</em>
</p>
