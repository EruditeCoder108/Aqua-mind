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

We use a **5-Stage Trust Architecture** to validate cheap sensor data before showing it to users. The result: **lab-aligned confidence** at a fraction of the cost.

**Tagline**: *"₹3,000 device with the wisdom of a lab scientist"*

---

## 📖 How It Works (Simple Explanation)

### The Big Idea

```
CHEAP SENSOR (₹300)  →  SMART SOFTWARE (FREE)  →  RELIABLE RESULTS
        ↓                        ↓                        ↓
   "350 ppm"              "Is this real?"           "CAUTION ⚠️"
   (might be wrong)       (5 trust checks)          (with confidence)
```

### Your Project Has 2 Parts

```
aqua-mind/
├── pi/           ← 🐍 Python code (runs on Raspberry Pi - the brain)
└── mobile-app/   ← 📱 Phone app (shows results beautifully - the face)
```

---

## 🐍 Part 1: Python Backend (The Brain)

### 1️⃣ `sensors.py` - The Eyes 👀

**What it does**: Reads raw numbers from TDS, Turbidity, and Temperature sensors.

**The cool part**: Has **SIMULATION MODE**! You can test without real hardware:
```python
clean_water  = { tds: 150, turbidity: 0.5, temp: 25 }  # Good water
dirty_water  = { tds: 650, turbidity: 8.0, temp: 30 }  # Bad water
sensor_error = { readings jumping randomly }           # Broken sensor
```

---

### 2️⃣ `trust_engine.py` - The Detective 🔍

This is the **HEART** of the project. Uses **5 tricks** to catch lying sensors:

#### Trick 1: Tri-Check (Take 3 tests, not 1)
```
❌ Wrong way:  Read once → 350 ppm (might be noise)
✅ Our way:    Read 3 times → 345, 352, 348 → Average: 348 ppm
```
If all 3 readings are close → Sensor is stable ✅  
If readings jump around → Sensor is unreliable ❌

#### Trick 2: Stability Score (0-100%)
| Score | Meaning | Action |
|-------|---------|--------|
| 95% | Very stable | Trust it completely |
| 70% | Okay | Probably fine |
| 40% | Unstable | Sensor might be dirty |
| <50% | Broken | Don't trust, clean probe! |

#### Trick 3: Geo-Profile (Location matters!)
Water problems in Jabalpur ≠ Jaipur ≠ Chennai

| City | Main Problem | We focus on... |
|------|--------------|----------------|
| Dhanwantri Nagar, Jabalpur | Sediment from Narmada | Turbidity (55%) |
| Jaipur | Desert = High minerals | TDS (70%) |
| Guwahati | Floods | Turbidity (70%) |
| Chennai | Coastal | Salinity/TDS (60%) |

#### Trick 4: Seasonal Awareness
```
January (winter)  → Normal thresholds
July (monsoon)    → Expect more sediment, adjust rules
```

#### Trick 5: Jal-Score (like CIBIL score for water!)
Combines everything into ONE number: **0-100**
- **80+** = SAFE ✅ पीने योग्य
- **50-80** = CAUTION ⚠️ सावधानी
- **<50** = UNSAFE 🚫 असुरक्षित

---

### 3️⃣ `rules_engine.py` - The Offline Doctor 💊

Gives safety advice **WITHOUT internet** (important for villages!):

```python
IF turbidity > 5 NTU:
    VERDICT = "UNSAFE"
    ACTION = "Boil water for 10 minutes"

IF stability < 50%:
    VERDICT = "ERROR"  
    ACTION = "Clean the sensor probe"
```

---

### 4️⃣ `bluetooth_comm.py` - The Messenger 📡

Sends results from Pi to your phone:
```
Raspberry Pi  →  Bluetooth  →  Phone App
                    ↓
            { tds: 350, verdict: "CAUTION", score: 72 }
```

---

### 5️⃣ `profiles.json` - The Map 🗺️

Stores settings for different cities:
```json
"DHANWANTRI_NAGAR": {
    "name": "Dhanwantri Nagar, Jabalpur",
    "tds_weight": 0.35,      // TDS matters 35%
    "turb_weight": 0.55,     // Turbidity matters 55%  
    "thresholds": {
        "tds_safe": 250,     // Below 250 = Safe
        "tds_caution": 400,  // 250-400 = Caution
        "tds_unsafe": 800    // Above 800 = Unsafe
    }
}
```

---

### 6️⃣ `main.py` - The Boss 👔

Connects everything:
```
1. Read sensors          →  "TDS = 350, Turb = 2.1"
2. Run Trust Engine      →  "Stability = 85%"
3. Apply Geo-Profile     →  "Jabalpur rules"
4. Calculate Jal-Score   →  "Score = 72"
5. Apply Rules Engine    →  "CAUTION + Actions"
6. Send to phone         →  📱 Display!
```

---

## 📱 Part 2: Mobile App (The Face)

| File | What It Does |
|------|--------------|
| `index.html` | Screen layout (buttons, cards) |
| `map.html` | 🗺️ **Ghost Map** - Network visualization |
| `style.css` | Beautiful dark blue theme |
| `app.js` | What happens when you click things |
| `bluetooth.js` | Connects to Raspberry Pi |
| `gemini.js` | Talks to Google AI |
| `sw.js` | Makes app work offline |

### What You See:
```
┌────────────────────────────┐
│        JAL-SCORE           │
│           72               │
│        ⚠️ CAUTION          │
├────────────────────────────┤
│  TDS        │  Turbidity   │
│  350 ppm    │  2.1 NTU     │
├────────────────────────────┤
│  🤖 AI SAYS:               │
│  "पानी में TDS ज़्यादा है"   │
│  Use RO filter recommended │
└────────────────────────────┘
```

---

## 🗺️ Ghost Map (Network Intelligence)

**The killer demo feature!** Shows how Aqua-Mind can scale to a city-wide network.

### What It Does:
- **Your Device** (Pulsing Cyan Dot) - Shows your live location
- **Network Devices** (Green/Orange/Red) - Simulated nearby Aqua-Mind users  
- **Contamination Cluster** (Red Polygon) - Detects infrastructure failures

### How It Works:
```
YOUR DEVICE (analyzing water)
       ↓
    [MAP VIEW]
       ↓
┌──────────────────────────────┐
│  🔵 Your Device (Live)       │
│  🟢🟢🟢 Safe Neighbors        │
│  🟠🟠 Caution Neighbors       │
│  🔴🔴🔴 Unsafe (RED ZONE!)    │
│     └── Polygon connects     │
│         these = CLUSTER      │
└──────────────────────────────┘
```

### For Judges (The Demo Trick):
> "The app visualizes **Networked Intelligence**. Here, you can see my device is analyzing (Cyan). Nearby, the system has detected a **cluster of high turbidity** (Red Zone), alerting the water department that a pipe has likely burst in Sector 4."

### Try It:
```
Open: mobile-app/map.html
```
└────────────────────────────┘
```

---

## 🤖 The AI Part (Gemini)

Takes numbers → Explains in simple Hindi/English:

**Input**: `TDS: 650, Turbidity: 3.2, Score: 58`

**Output**: 
> "पानी में TDS ज़्यादा है (High minerals detected).  
> This is like dissolved chalk - not immediately harmful, but may cause kidney stones over years.  
> **ACTION**: Use RO filter or mix with RO water."

---

## 🔄 Complete Data Flow

```
WATER SAMPLE
     ↓
[SENSORS] ──────────→ Raw numbers (TDS=650, Turb=3.2)
     ↓
[TRI-CHECK] ────────→ Take 3 readings, average them
     ↓
[STABILITY CHECK] ──→ Is sensor reliable? (85% = Yes)
     ↓
[GEO-PROFILE] ──────→ Apply Jabalpur-specific rules
     ↓
[JAL-SCORE] ────────→ Calculate final score (58/100)
     ↓
[RULES ENGINE] ─────→ Verdict: CAUTION, Actions list
     ↓
[BLUETOOTH] ────────→ Send to phone
     ↓
[MOBILE APP] ───────→ Display beautifully
     ↓
[GEMINI AI] ────────→ Explain: "पानी ठीक नहीं है"
```

---

## 🎯 Why This Beats Other Projects

| Cheap Sensors Alone | Aqua-Mind |
|---------------------|-----------|
| Give raw numbers | Gives Jal-Score + Verdict |
| No error detection | Tri-Check catches noise |
| Same rules everywhere | Geo-adaptive for each city |
| Needs internet | Works fully offline |
| English only | Hindi + English |
| Just data | Actionable advice |

---

## 🚀 Quick Start

### Test Without Hardware (Right Now!)

```bash
# Clone the repository
git clone https://github.com/EruditeCoder108/Aqua-mind.git
cd Aqua-mind/pi

# Try different water scenarios
python main.py --scenario clean_water --single    # Good water
python main.py --scenario tap_water --single      # Normal tap
python main.py --scenario dirty_water --single    # Bad water
python main.py --scenario sensor_error --single   # Broken sensor

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

    ⚠️ CRITICAL: Use voltage divider (2× 10kΩ) for Turbidity!
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
python3 main.py --profile DHANWANTRI_NAGAR
```

---

## 📁 Project Structure

```
Aqua-mind/
├── pi/                         # Raspberry Pi backend
│   ├── main.py                 # Main orchestrator (The Boss)
│   ├── sensors.py              # Hardware drivers (The Eyes)
│   ├── trust_engine.py         # 5-Pillar Trust System (The Detective)
│   ├── rules_engine.py         # Offline safety rules (The Doctor)
│   ├── bluetooth_comm.py       # Bluetooth serial (The Messenger)
│   ├── profiles.json           # Regional configurations (The Map)
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
    └── sw.js                   # Service worker (offline)
```

---

## 🌍 Regional Profiles

| Region | Primary Concern | TDS Weight | Turbidity Weight |
|--------|-----------------|------------|------------------|
| **Dhanwantri Nagar, Jabalpur** | Narmada sediment | 35% | 55% |
| **Jabalpur City, MP** | Monsoon sediment | 30% | 60% |
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
- **Research Reference**: IJRPR31819 - Analysis of Drinking Water in Jabalpur City

---

## 🛣️ Roadmap

- [x] Core Python backend with simulation
- [x] 5-Pillar Trust System
- [x] Mobile PWA with Gemini AI
- [x] Regional profiles (7 locations)
- [x] Dhanwantri Nagar research-backed profile
- [ ] Hardware integration testing
- [ ] Sensor calibration interface
- [ ] Data export (CSV/PDF reports)
- [ ] Multi-language support

---

## 👨‍💻 Author

**Jal Jeevan Mission Innovation Challenge Entry**  
📍 Dhanwantri Nagar, Jabalpur, Madhya Pradesh

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

---

<p align="center">
  <strong>🌊 Clean Water for All | जल जीवन मिशन 🇮🇳</strong>
</p>
<p align="center">
  <em>"Lab-aligned confidence at ₹3,000"</em>
</p>
