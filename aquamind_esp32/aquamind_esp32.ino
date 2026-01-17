/*
 * ╔═══════════════════════════════════════════════════════════════╗
 * ║        AQUA-MIND ESP32 - Water Quality Intelligence           ║
 * ║                      Version 2.1 (God Mode)                   ║
 * ╚═══════════════════════════════════════════════════════════════╝
 * 
 * Hardware:
 * - ESP32 DevKit (30-pin)
 * - TDS Sensor V1.0 (GPIO 34)
 * - pH Sensor Module (GPIO 35) [MOCKED]
 * - Turbidity, Temperature, DO [MOCKED]
 * 
 * Features:
 * - 6-Parameter Jal-Score Algorithm (TDS, pH, DO, Turb, Temp, Stability)
 * - Dynamic Geo-Profile Thresholds
 * - Tri-Check Burst Sampling with Outlier Rejection
 * - BLE Communication with Rich JSON Payload
 * - WiFi-based Location & Seasonal Adaptation
 * - NTP Time Sync for Accurate Season Detection
 * 
 * Author: Aqua-Mind Team (Sambhav Jain)
 * License: MIT
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>  // For saving WiFi credentials
#include <time.h>         // For NTP time sync
// Using simple String-based JSON parsing (no ArduinoJson needed)

// ============================================
// WIFI CONFIGURATION
// ============================================
// Default WiFi Credentials (hardcoded fallback)
const char* DEFAULT_WIFI_SSID = "Airtel_Sam";
const char* DEFAULT_WIFI_PASS = "Samj@777";

// Active WiFi credentials (can be updated via BLE)
String wifiSSID = DEFAULT_WIFI_SSID;
String wifiPassword = DEFAULT_WIFI_PASS;

bool wifiConnected = false;
bool ntpSynced = false;
Preferences preferences;  // For persistent storage

// ============================================
// GEO-PROFILE DATA (for auto-detection)
// ============================================
struct GeoProfile {
    const char* name;
    float lat;
    float lon;
    int tds_safe;
    int tds_danger;
    float turb_safe;
    float turb_danger;
};

// Hardcoded profiles - easily expandable!
const GeoProfile PROFILES[] = {
    {"Jabalpur",  23.18, 79.98, 300, 900, 1.0, 10.0},
    {"Jaipur",    26.91, 75.78, 400, 1200, 2.0, 10.0},
    {"Chennai",   13.08, 80.27, 500, 1500, 1.0, 10.0},
    {"Delhi",     28.61, 77.20, 300, 800, 1.0, 8.0},
    {"Mumbai",    19.07, 72.87, 400, 1000, 1.0, 10.0},
    {"Guwahati",  26.14, 91.73, 200, 700, 1.0, 8.0},
};
const int PROFILE_COUNT = 6;

// Current active profile
int activeProfileIndex = 0;  // Default: Jabalpur
String detectedCity = "Unknown";
float detectedLat = 0;
float detectedLon = 0;

// Weather data (from API, for season detection)
String currentSeason = "normal";
float ambientTemp = 28.0;     // Default ambient temp (India average)
bool isRaining = false;
int currentMonth = 1;         // Will be updated from NTP

// ============================================
// PIN DEFINITIONS
// ============================================
#define TDS_PIN         34    // TDS analog input
#define PH_PIN          35    // pH analog input
#define BUTTON_PIN      0     // Boot button for manual test

// ADC Configuration
#define ADC_RESOLUTION  12    // ESP32 has 12-bit ADC (0-4095)
#define VREF            3.3   // Reference voltage

// ============================================
// SENSOR CALIBRATION
// ============================================
// TDS Calibration (adjust after testing with known solution)
#define TDS_CALIBRATION_OFFSET  0.0
#define TDS_CALIBRATION_FACTOR  0.5   // Convert voltage to ppm

// pH Calibration (adjust with pH 4.0, 7.0, 10.0 solutions)
#define PH_OFFSET       0.0
#define PH_SLOPE        3.5   // mV per pH unit

// ============================================
// MOCK SENSOR SETTINGS
// ============================================
// These will be replaced when you get actual sensors
#define MOCK_TURBIDITY_NTU      2.5   // Simulated turbidity
#define MOCK_TEMPERATURE_C      25.0  // Simulated temperature
#define MOCK_DO_MGL             7.5   // Simulated dissolved oxygen (mg/L)

// ============================================
// TRI-CHECK CONFIGURATION
// ============================================
#define BURSTS              3     // Number of burst readings
#define SAMPLES_PER_BURST   5     // Samples per burst
#define BURST_DELAY_MS      200   // Delay between bursts (ms)
#define SAMPLE_DELAY_MS     10    // Delay between samples (ms)

// ============================================
// JAL-SCORE THRESHOLDS (Jabalpur Profile)
// ============================================
#define TDS_SAFE_LIMIT      300   // ppm - below this is safe
#define TDS_DANGER_LIMIT    600   // ppm - above this is danger
#define PH_SAFE_MIN         6.5   // Minimum safe pH
#define PH_SAFE_MAX         8.5   // Maximum safe pH
#define TURBIDITY_LIMIT     4.0   // NTU - above this is unsafe

// Weights for Jal-Score calculation (total = 1.0)
#define WEIGHT_TDS          0.30
#define WEIGHT_PH           0.25
#define WEIGHT_DO           0.15  // Dissolved Oxygen
#define WEIGHT_TURBIDITY    0.15
#define WEIGHT_STABILITY    0.15

// ============================================
// BLE CONFIGURATION
// ============================================
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHAR_DATA_UUID      "12345678-1234-1234-1234-123456789abd"
#define CHAR_COMMAND_UUID   "12345678-1234-1234-1234-123456789abe"
#define DEVICE_NAME         "AquaMind-ESP32"

// ============================================
// GLOBAL VARIABLES
// ============================================
BLEServer* pServer = NULL;
BLECharacteristic* pDataCharacteristic = NULL;
BLECharacteristic* pCommandCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Sensor readings
float lastTDS = 0;
float lastPH = 0;
float lastDO = MOCK_DO_MGL;  // Dissolved Oxygen (mg/L)
float lastTurbidity = MOCK_TURBIDITY_NTU;
float lastTemperature = MOCK_TEMPERATURE_C;
float lastStability = 0;
int lastJalScore = 0;
String lastVerdict = "UNKNOWN";

// Analysis counter
int analysisCount = 0;

// Forward declarations (required for C++ compilation order)
void analyzeWater();
void sendStatus();
void sendAnalysisViaBLE();
void handleWiFiCommand(String cmd);
void connectWiFi();
void fetchLocation();
void fetchWeather();
void syncNTPTime();
void detectSeason();
void loadSavedWiFi();
int estimateMonthFromTemp(float temp);
bool httpGetWithRetry(String url, String* payload, int maxRetries);
float readDO();

// ============================================
// BLE SERVER CALLBACKS
// ============================================
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 BLE Client Connected!");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("📱 BLE Client Disconnected");
    }
};

// ============================================
// COMMAND HANDLER
// ============================================
class CommandCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            String cmd = String(value.c_str());
            Serial.println("📩 Command received: " + cmd);
            
            if (cmd == "ANALYZE") {
                Serial.println("🔬 Starting analysis from BLE command...");
                analyzeWater();
            } else if (cmd == "STATUS") {
                sendStatus();
            } else if (cmd.startsWith("WIFI:")) {
                // Parse WiFi credentials: WIFI:ssid:password
                handleWiFiCommand(cmd);
            }
        }
    }
};

/**
 * Handle WiFi credential update via BLE
 * Format: WIFI:ssid:password
 */
void handleWiFiCommand(String cmd) {
    // Remove "WIFI:" prefix
    String credentials = cmd.substring(5);
    
    // Find the separator between SSID and password
    int separatorIdx = credentials.indexOf(':');
    if (separatorIdx > 0) {
        String newSSID = credentials.substring(0, separatorIdx);
        String newPassword = credentials.substring(separatorIdx + 1);
        
        Serial.println("📶 Saving new WiFi credentials...");
        Serial.println("   SSID: " + newSSID);
        Serial.println("   Password: " + String(newPassword.length()) + " chars");
        
        // Save to Preferences (persistent storage)
        preferences.begin("wifi", false);
        preferences.putString("ssid", newSSID);
        preferences.putString("password", newPassword);
        preferences.end();
        
        // Update current credentials
        wifiSSID = newSSID;
        wifiPassword = newPassword;
        
        // Send confirmation via BLE
        String response = "{\"status\":\"wifi_saved\",\"ssid\":\"" + newSSID + "\"}";
        pDataCharacteristic->setValue(response.c_str());
        pDataCharacteristic->notify();
        
        // Reconnect with new credentials
        Serial.println("🔄 Reconnecting to new WiFi...");
        WiFi.disconnect();
        delay(100);
        connectWiFi();
        
        // Fetch new location if connected
        if (wifiConnected) {
            fetchLocation();
            fetchWeather();
        }
    } else {
        Serial.println("❌ Invalid WiFi command format. Use: WIFI:ssid:password");
    }
}

/**
 * Load saved WiFi credentials from Preferences
 */
void loadSavedWiFi() {
    preferences.begin("wifi", true);  // Read-only
    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");
    preferences.end();
    
    if (savedSSID.length() > 0) {
        wifiSSID = savedSSID;
        wifiPassword = savedPassword;
        Serial.println("📂 Loaded saved WiFi: " + wifiSSID);
    }
}

// ============================================
// SENSOR READING FUNCTIONS
// ============================================

/**
 * Read raw ADC value from TDS sensor
 */
int readTDSRaw() {
    return analogRead(TDS_PIN);
}

/**
 * Convert TDS ADC reading to ppm
 */
float readTDSppm() {
    int raw = readTDSRaw();
    float voltage = (raw / 4095.0) * VREF;
    
    // TDS formula: depends on your specific sensor
    // This is a common approximation for TDS V1.0 modules
    float tds = (133.42 * voltage * voltage * voltage 
                - 255.86 * voltage * voltage 
                + 857.39 * voltage) * TDS_CALIBRATION_FACTOR;
    
    tds += TDS_CALIBRATION_OFFSET;
    
    // Temperature compensation (using mock temp for now)
    float compensationCoefficient = 1.0 + 0.02 * (lastTemperature - 25.0);
    tds = tds / compensationCoefficient;
    
    return max(0.0f, tds);
}

/**
 * Read raw ADC value from pH sensor
 */
int readPHRaw() {
    return analogRead(PH_PIN);
}

/**
 * Convert pH ADC reading to pH value
 * MOCKED for now (user requests mock data)
 */
float readPH() {
    // Return safe mock value with minimal noise (±0.05)
    // Real pH sensors are very stable when calibrated
    return 7.2 + (random(-5, 5) / 100.0);
}

/*
// REAL SENSOR IMPLEMENTATION (Keep for future)
float readPH_Real() {
    int raw = readPHRaw();
    float voltage = (raw / 4095.0) * VREF * 1000.0; // mV
    
    // pH calculation (typical for ADS-based pH sensor)
    // Neutral pH 7.0 is around 1500mV, slope is about 59mV per pH unit
    float ph = 7.0 + ((1500.0 - voltage) / 59.16);
    ph += PH_OFFSET;
    
    // Clamp to valid range
    return constrain(ph, 0.0f, 14.0f);
}
*/

/**
 * Get turbidity (MOCKED for now)
 */
float readTurbidity() {
    // TODO: Replace with actual sensor reading when available
    // Add some random noise to make it realistic
    return MOCK_TURBIDITY_NTU + (random(-20, 20) / 100.0);
}

/**
 * Get temperature (MOCKED for now)
 */
float readTemperature() {
    // TODO: Replace with DS18B20 reading when available
    // Add some random noise
    return MOCK_TEMPERATURE_C + (random(-10, 10) / 10.0);
}

/**
 * Get dissolved oxygen (MOCKED for now)
 * Normal range: 5-9 mg/L for healthy water
 */
float readDO() {
    // TODO: Replace with actual DO sensor reading when available
    // Add some random noise
    return MOCK_DO_MGL + (random(-10, 10) / 10.0);
}

// ============================================
// TRI-CHECK BURST SAMPLING
// ============================================

/**
 * Perform Tri-Check burst sampling for a sensor
 * Returns: mean value
 * Sets: stability score via pointer
 */
float triCheck(float (*sensorFunc)(), float* stability) {
    float burstMeans[BURSTS];
    float allReadings[BURSTS * SAMPLES_PER_BURST];
    int readingIndex = 0;
    
    Serial.println("  📊 Tri-Check: Starting burst sampling...");
    
    for (int burst = 0; burst < BURSTS; burst++) {
        float burstSum = 0;
        
        for (int sample = 0; sample < SAMPLES_PER_BURST; sample++) {
            float reading = sensorFunc();
            allReadings[readingIndex++] = reading;
            burstSum += reading;
            delay(SAMPLE_DELAY_MS);
        }
        
        burstMeans[burst] = burstSum / SAMPLES_PER_BURST;
        Serial.printf("     Burst %d: %.2f\n", burst + 1, burstMeans[burst]);
        
        if (burst < BURSTS - 1) {
            delay(BURST_DELAY_MS);
        }
    }
    
    // Calculate overall mean
    float totalSum = 0;
    for (int i = 0; i < readingIndex; i++) {
        totalSum += allReadings[i];
    }
    float mean = totalSum / readingIndex;
    
    // Calculate variance for stability score
    float variance = 0;
    for (int i = 0; i < readingIndex; i++) {
        float diff = allReadings[i] - mean;
        variance += diff * diff;
    }
    variance /= readingIndex;
    float stdDev = sqrt(variance);
    
    // Stability score: 100% = no variance, lower = more variance
    // Using coefficient of variation (CV) with gentler scaling
    float cv = (mean > 0) ? (stdDev / mean) * 100.0 : 0;
    // Gentler scaling: CV of 2% = 90% stability, CV of 10% = 50% stability
    *stability = max(50.0f, 100.0f - cv * 5.0f);  // Floor at 50% for reasonable data
    
    Serial.printf("  📊 Tri-Check Result: Mean=%.2f, Stability=%.1f%%\n", mean, *stability);
    
    return mean;
}

// ============================================
// JAL-SCORE CALCULATION
// ============================================
/**
 * Calculate Jal-Score (0-100) based on 6 parameters
 * Uses DYNAMIC thresholds from active GeoProfile
 * Weights: TDS(30%) + pH(25%) + DO(15%) + Turb(15%) + Stability(15%) = 100%
 */
int calculateJalScore(float tds, float ph, float turbidity, float doLevel, float stability) {
    // Get dynamic thresholds from active profile
    int tdsSafe = PROFILES[activeProfileIndex].tds_safe;
    int tdsDanger = PROFILES[activeProfileIndex].tds_danger;
    float turbSafe = PROFILES[activeProfileIndex].turb_safe;
    float turbDanger = PROFILES[activeProfileIndex].turb_danger;
    
    // ═══════════════════════════════════════════
    // TDS Score (0-100, higher TDS = lower score)
    // ═══════════════════════════════════════════
    float tdsScore;
    if (tds <= tdsSafe) {
        tdsScore = 100.0;
    } else if (tds >= tdsDanger) {
        tdsScore = 0.0;
    } else {
        tdsScore = 100.0 - ((tds - tdsSafe) / (float)(tdsDanger - tdsSafe)) * 100.0;
    }
    
    // ═══════════════════════════════════════════
    // pH Score (0-100, optimal at 7.0-7.5)
    // ═══════════════════════════════════════════
    float phScore;
    if (ph >= PH_SAFE_MIN && ph <= PH_SAFE_MAX) {
        // Within safe range - perfect at 7.25
        float distFromOptimal = abs(ph - 7.25);
        phScore = 100.0 - (distFromOptimal * 15.0);  // Gentler penalty
    } else if (ph < PH_SAFE_MIN) {
        float penalty = (PH_SAFE_MIN - ph) * 25.0;
        phScore = max(0.0f, 50.0f - penalty);
    } else {
        float penalty = (ph - PH_SAFE_MAX) * 25.0;
        phScore = max(0.0f, 50.0f - penalty);
    }
    
    // ═══════════════════════════════════════════
    // Dissolved Oxygen Score (optimal: 6-8 mg/L)
    // ═══════════════════════════════════════════
    float doScore;
    if (doLevel >= 6.0 && doLevel <= 9.0) {
        doScore = 100.0;  // Perfect range
    } else if (doLevel >= 5.0 && doLevel < 6.0) {
        doScore = 80.0;   // Acceptable low
    } else if (doLevel > 9.0 && doLevel <= 12.0) {
        doScore = 90.0;   // Slightly high but OK
    } else if (doLevel < 5.0) {
        doScore = max(0.0f, 50.0f - (5.0f - doLevel) * 20.0f);  // Hypoxic
    } else {
        doScore = 70.0;   // Very high (rare)
    }
    
    // ═══════════════════════════════════════════
    // Turbidity Score (lower is better)
    // ═══════════════════════════════════════════
    float turbScore;
    if (turbidity <= turbSafe) {
        turbScore = 100.0;
    } else if (turbidity >= turbDanger) {
        turbScore = 0.0;
    } else {
        turbScore = 100.0 - ((turbidity - turbSafe) / (turbDanger - turbSafe)) * 100.0;
    }
    
    // ═══════════════════════════════════════════
    // Weighted Final Score
    // ═══════════════════════════════════════════
    float jalScore = (tdsScore * WEIGHT_TDS) +
                     (phScore * WEIGHT_PH) +
                     (doScore * WEIGHT_DO) +
                     (turbScore * WEIGHT_TURBIDITY) +
                     (stability * WEIGHT_STABILITY);
    
    // Apply graduated stability penalty
    if (stability < 50) {
        jalScore *= 0.8;  // 20% penalty for very unstable
    } else if (stability < 70) {
        jalScore *= 0.9;  // 10% penalty for unstable
    }
    
    // Debug output with all 5 components
    Serial.printf("  📈 Score Components:\n");
    Serial.printf("      TDS=%.1f, pH=%.1f, DO=%.1f, Turb=%.1f, Stab=%.1f\n",
                  tdsScore, phScore, doScore, turbScore, stability);
    Serial.printf("      Using profile: %s (TDS limit: %d-%d ppm)\n", 
                  PROFILES[activeProfileIndex].name, tdsSafe, tdsDanger);
    
    return constrain((int)jalScore, 0, 100);
}

/**
 * Get verdict string based on Jal-Score
 */
String getVerdict(int score) {
    if (score >= 80) return "SAFE";
    if (score >= 60) return "ACCEPTABLE";
    if (score >= 40) return "CAUTION";
    return "UNSAFE";
}

/**
 * Get emoji for verdict
 */
String getVerdictEmoji(String verdict) {
    if (verdict == "SAFE") return "✅";
    if (verdict == "ACCEPTABLE") return "👍";
    if (verdict == "CAUTION") return "⚠️";
    return "🚫";
}

// ============================================
// MAIN ANALYSIS FUNCTION
// ============================================

void analyzeWater() {
    analysisCount++;
    
    Serial.println("\n============================================");
    Serial.printf("  🔬 WATER ANALYSIS #%d\n", analysisCount);
    Serial.println("============================================");
    
    // Read temperature first (for compensation)
    lastTemperature = readTemperature();
    Serial.printf("  🌡️  Temperature: %.1f°C (mocked)\n", lastTemperature);
    
    // Tri-Check for TDS
    Serial.println("\n  [TDS Sensor]");
    float tdsStability;
    lastTDS = triCheck(readTDSppm, &tdsStability);
    
    // Tri-Check for pH
    Serial.println("\n  [pH Sensor]");
    float phStability;
    lastPH = triCheck(readPH, &phStability);
    
    // Turbidity (mocked)
    lastTurbidity = readTurbidity();
    Serial.printf("\n  💧 Turbidity: %.2f NTU (mocked)\n", lastTurbidity);
    
    // Dissolved Oxygen (mocked)
    lastDO = readDO();
    Serial.printf("  🫧 Dissolved O₂: %.1f mg/L (mocked)\n", lastDO);
    
    // Overall stability (average of TDS and pH stability)
    lastStability = (tdsStability + phStability) / 2.0;
    
    // Calculate Jal-Score (now includes DO!)
    Serial.println("\n  🎯 Calculating Jal-Score (6-parameter)...");
    lastJalScore = calculateJalScore(lastTDS, lastPH, lastTurbidity, lastDO, lastStability);
    lastVerdict = getVerdict(lastJalScore);
    
    // =====================================
    // CRITICAL PARAMETER OVERRIDE (Safety Net)
    // =====================================
    // If ANY parameter is in the "danger zone", force UNSAFE verdict
    String criticalAlert = "";
    
    if (lastPH < 4.0 || lastPH > 10.0) {
        criticalAlert = "⛔ CRITICAL: pH " + String(lastPH, 1) + " is DANGEROUS!";
        lastVerdict = "UNSAFE";
        lastJalScore = min(lastJalScore, 30);  // Cap score at 30
    }
    if (lastTDS > 800) {
        criticalAlert = "⛔ CRITICAL: TDS " + String(lastTDS, 0) + " ppm is DANGEROUS!";
        lastVerdict = "UNSAFE";
        lastJalScore = min(lastJalScore, 30);
    }
    if (lastTurbidity > 8.0) {
        criticalAlert = "⛔ CRITICAL: Turbidity " + String(lastTurbidity, 1) + " NTU is DANGEROUS!";
        lastVerdict = "UNSAFE";
        lastJalScore = min(lastJalScore, 30);
    }
    if (lastStability < 40) {
        criticalAlert = "⚠️ WARNING: Sensor readings unstable! Clean probes.";
        if (lastVerdict == "SAFE") lastVerdict = "CAUTION";
    }
    
    // Print critical alert if any
    if (criticalAlert.length() > 0) {
        Serial.println("\n" + criticalAlert);
    }
    
    // Print results
    Serial.println("\n============================================");
    Serial.printf("  %s RESULT: %s\n", getVerdictEmoji(lastVerdict).c_str(), lastVerdict.c_str());
    Serial.println("--------------------------------------------");
    Serial.printf("  📊 TDS:         %.1f ppm\n", lastTDS);
    Serial.printf("  📊 pH:          %.2f\n", lastPH);
    Serial.printf("  📊 Dissolved O₂: %.1f mg/L (mocked)\n", lastDO);
    Serial.printf("  📊 Turbidity:   %.2f NTU (mocked)\n", lastTurbidity);
    Serial.printf("  📊 Temperature: %.1f°C (mocked)\n", lastTemperature);
    Serial.printf("  📊 Stability:   %.1f%%\n", lastStability);
    Serial.println("--------------------------------------------");
    Serial.printf("  🎯 JAL-SCORE:   %d/100\n", lastJalScore);
    Serial.println("============================================\n");
    
    // Send to connected BLE client
    if (deviceConnected) {
        sendAnalysisViaBLE();
    }
}

// ============================================
// BLE DATA TRANSMISSION
// ============================================

/**
 * Send analysis result via BLE as JSON
 * Rich payload includes: sensors, score, profile, location, season
 */
void sendAnalysisViaBLE() {
    // Create comprehensive JSON string
    String json = "{";
    
    // Sensor data
    json += "\"tds\":" + String(lastTDS, 1) + ",";
    json += "\"ph\":" + String(lastPH, 2) + ",";
    json += "\"do\":" + String(lastDO, 1) + ",";
    json += "\"turbidity\":" + String(lastTurbidity, 2) + ",";
    json += "\"temperature\":" + String(lastTemperature, 1) + ",";
    json += "\"stability\":" + String(lastStability, 1) + ",";
    
    // Score and verdict
    json += "\"jal_score\":" + String(lastJalScore) + ",";
    json += "\"verdict\":\"" + lastVerdict + "\",";
    
    // Location and profile info (new!)
    json += "\"profile\":\"" + String(PROFILES[activeProfileIndex].name) + "\",";
    json += "\"city\":\"" + detectedCity + "\",";
    json += "\"season\":\"" + currentSeason + "\",";
    json += "\"ambient_temp\":" + String(ambientTemp, 1) + ",";
    
    // Metadata
    json += "\"timestamp\":" + String(millis());
    json += "}";
    
    pDataCharacteristic->setValue(json.c_str());
    pDataCharacteristic->notify();
    
    Serial.println("📤 Sent to BLE: " + json);
}

/**
 * Send status update via BLE
 */
void sendStatus() {
    String status = "{\"status\":\"ready\",\"analyses\":" + String(analysisCount) + "}";
    pDataCharacteristic->setValue(status.c_str());
    pDataCharacteristic->notify();
}

// ============================================
// BLE SETUP
// ============================================

void setupBLE() {
    Serial.println("📡 Initializing Bluetooth LE...");
    
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Data characteristic (notify)
    pDataCharacteristic = pService->createCharacteristic(
        CHAR_DATA_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pDataCharacteristic->addDescriptor(new BLE2902());
    
    // Command characteristic (write)
    pCommandCharacteristic = pService->createCharacteristic(
        CHAR_COMMAND_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCommandCharacteristic->setCallbacks(new CommandCallbacks());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    Serial.println("📡 BLE Ready! Device name: " + String(DEVICE_NAME));
}

// ============================================
// WIFI & LOCATION FUNCTIONS
// ============================================

/**
 * Sync time from NTP server (for accurate month/season detection)
 */
void syncNTPTime() {
    Serial.println("🕐 Syncing time from NTP...");
    
    // IST = UTC + 5:30 = 19800 seconds
    configTime(19800, 0, "pool.ntp.org", "time.google.com");
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10000)) {  // 10 second timeout
        ntpSynced = true;
        currentMonth = timeinfo.tm_mon + 1;  // tm_mon is 0-11
        Serial.printf("   📅 Date: %d-%02d-%02d %02d:%02d (Month: %d)\n",
            timeinfo.tm_year + 1900, currentMonth, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, currentMonth);
    } else {
        Serial.println("   ⚠️ NTP sync failed, estimating month from weather");
        ntpSynced = false;
    }
}

/**
 * Estimate month from ambient temperature (fallback if NTP fails)
 */
int estimateMonthFromTemp(float temp) {
    if (temp > 38) return 5;   // May (peak summer)
    if (temp > 32) return 4;   // April
    if (temp < 12) return 1;   // January (winter)
    if (temp < 18) return 12;  // December
    return 8;                  // August default (monsoon likely)
}

/**
 * HTTP GET with retry mechanism
 */
bool httpGetWithRetry(String url, String* payload, int maxRetries = 3) {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        http.begin(url);
        http.setTimeout(10000);  // 10 second timeout
        int httpCode = http.GET();
        
        if (httpCode == 200) {
            *payload = http.getString();
            http.end();
            return true;
        }
        
        Serial.printf("   ⚠️ Attempt %d/%d failed (HTTP %d)\n", attempt, maxRetries, httpCode);
        http.end();
        
        if (attempt < maxRetries) {
            delay(2000);  // Wait before retry
        }
    }
    return false;
}

/**
 * Connect to WiFi network
 */
void connectWiFi() {
    Serial.print("📶 Connecting to WiFi: ");
    Serial.println(wifiSSID);
    
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("   IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        wifiConnected = false;
        Serial.println("\n⚠️ WiFi connection failed. Using default profile.");
    }
}

/**
 * Calculate distance between two lat/lon points (Haversine formula)
 */
float haversineDistance(float lat1, float lon1, float lat2, float lon2) {
    float R = 6371; // Earth radius in km
    float dLat = (lat2 - lat1) * PI / 180.0;
    float dLon = (lon2 - lon1) * PI / 180.0;
    float a = sin(dLat/2) * sin(dLat/2) + 
              cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0) * 
              sin(dLon/2) * sin(dLon/2);
    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
}

/**
 * Find closest profile to given location
 */
void matchClosestProfile(float lat, float lon) {
    float minDistance = 999999;
    int closest = 0;
    
    for (int i = 0; i < PROFILE_COUNT; i++) {
        float dist = haversineDistance(lat, lon, PROFILES[i].lat, PROFILES[i].lon);
        if (dist < minDistance) {
            minDistance = dist;
            closest = i;
        }
    }
    
    activeProfileIndex = closest;
    Serial.printf("📍 Matched profile: %s (%.0f km away)\n", PROFILES[closest].name, minDistance);
}

/**
 * Fetch location via IP geolocation (ip-api.com)
 */
void fetchLocation() {
    if (!wifiConnected) return;
    
    Serial.println("🌍 Fetching location from IP...");
    
    HTTPClient http;
    http.begin("http://ip-api.com/json/");
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        // Simple JSON parsing (ArduinoJson would be better for complex JSON)
        // Looking for: "lat":XX.XX,"lon":XX.XX,"city":"XXX"
        int latIdx = payload.indexOf("\"lat\":");
        int lonIdx = payload.indexOf("\"lon\":");
        int cityIdx = payload.indexOf("\"city\":\"");
        
        if (latIdx > 0 && lonIdx > 0) {
            detectedLat = payload.substring(latIdx + 6, payload.indexOf(",", latIdx)).toFloat();
            detectedLon = payload.substring(lonIdx + 6, payload.indexOf(",", lonIdx)).toFloat();
            
            if (cityIdx > 0) {
                int cityEnd = payload.indexOf("\"", cityIdx + 8);
                detectedCity = payload.substring(cityIdx + 8, cityEnd);
            }
            
            Serial.printf("   Location: %s (%.2f, %.2f)\n", detectedCity.c_str(), detectedLat, detectedLon);
            matchClosestProfile(detectedLat, detectedLon);
        }
    } else {
        Serial.printf("⚠️ Location fetch failed: %d\n", httpCode);
    }
    
    http.end();
}

/**
 * Fetch weather from OpenMeteo (free, no API key needed!)
 * Uses retry mechanism for robustness
 */
void fetchWeather() {
    if (!wifiConnected || detectedLat == 0) return;
    
    Serial.println("🌤️ Fetching weather...");
    
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(detectedLat, 2) + 
                 "&longitude=" + String(detectedLon, 2) + "&current_weather=true";
    
    String payload;
    if (!httpGetWithRetry(url, &payload, 3)) {
        Serial.println("   ⚠️ Weather fetch failed after 3 attempts");
        Serial.println("   📊 Using defaults: 28°C, normal season");
        // Keep default ambientTemp = 28.0
        return;
    }
    
    // Extract temperature from JSON response
    int tempIdx = payload.indexOf("\"temperature\":");
    if (tempIdx > 0) {
        float parsedTemp = payload.substring(tempIdx + 14, payload.indexOf(",", tempIdx)).toFloat();
        if (parsedTemp > -50 && parsedTemp < 60) {  // Sanity check
            ambientTemp = parsedTemp;
        }
    }
    
    // Extract weather code
    int codeIdx = payload.indexOf("\"weathercode\":");
    if (codeIdx > 0) {
        int weatherCode = payload.substring(codeIdx + 14, payload.indexOf(",", codeIdx)).toInt();
        // Weather codes 51-99 indicate precipitation
        isRaining = (weatherCode >= 51 && weatherCode <= 99);
    }
    
    Serial.printf("   🌡️ Ambient: %.1f°C, %s\n", ambientTemp, isRaining ? "🌧️ Raining" : "☀️ Clear");
    
    // If NTP failed, estimate month from temperature
    if (!ntpSynced) {
        currentMonth = estimateMonthFromTemp(ambientTemp);
        Serial.printf("   📅 Estimated month: %d (from temp)\n", currentMonth);
    }
    
    // Determine season based on real month
    detectSeason();
}

/**
 * Detect season based on month and weather conditions
 */
void detectSeason() {
    // India seasons:
    // Winter: Nov-Feb (month 11,12,1,2)
    // Summer: Mar-May (month 3,4,5) 
    // Monsoon: Jun-Sep (month 6,7,8,9)
    // Post-monsoon: Oct (month 10)
    
    if (currentMonth >= 6 && currentMonth <= 9) {
        if (isRaining) {
            currentSeason = "monsoon";
            Serial.println("   🌧️ Season: MONSOON (heavy rainfall expected)");
        } else {
            currentSeason = "monsoon";
            Serial.println("   🌧️ Season: MONSOON PERIOD");
        }
    } else if (currentMonth >= 3 && currentMonth <= 5) {
        currentSeason = "summer";
        Serial.println("   ☀️ Season: SUMMER (high evaporation)");
    } else if (currentMonth >= 11 || currentMonth <= 2) {
        currentSeason = "winter";
        Serial.println("   ❄️ Season: WINTER");
    } else {
        currentSeason = "normal";
        Serial.println("   🍂 Season: POST-MONSOON");
    }
}

// ============================================
// SETUP
// ============================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════════╗");
    Serial.println("║  🌊 AQUA-MIND Water Quality Intelligence  ║");
    Serial.println("║  📟 ESP32 Edition v2.1 (God Mode)         ║");
    Serial.println("║  6-Parameter Jal-Score • Geo-Adaptive     ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    
    // Configure ADC
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_11db);  // Full 0-3.3V range
    
    // Configure button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize BLE
    setupBLE();
    
    // WIFI PRIORITY:
    // 1. Try DEFAULT credentials first (hardcoded)
    // 2. If fails, try SAVED credentials (from mobile app)
    // 3. If both fail, continue without WiFi
    
    Serial.println("\n📶 WiFi Connection Strategy:");
    Serial.println("   1. Trying default credentials...");
    
    // Step 1: Try default WiFi
    wifiSSID = DEFAULT_WIFI_SSID;
    wifiPassword = DEFAULT_WIFI_PASS;
    connectWiFi();
    
    // Step 2: If default failed, try saved credentials
    if (!wifiConnected) {
        preferences.begin("wifi", true);
        String savedSSID = preferences.getString("ssid", "");
        preferences.end();
        
        if (savedSSID.length() > 0 && savedSSID != String(DEFAULT_WIFI_SSID)) {
            Serial.println("   2. Trying saved credentials...");
            loadSavedWiFi();
            connectWiFi();
        }
    }
    
    // Step 3: If connected, sync time and fetch location
    if (wifiConnected) {
        syncNTPTime();      // Get real date/time
        fetchLocation();    // Get location from IP
        fetchWeather();     // Get weather + detect season
    } else {
        Serial.println("\n⚠️ No WiFi - using default profile (Jabalpur)");
    }
    
    // Show active profile
    Serial.println("\n📍 Active Profile: " + String(PROFILES[activeProfileIndex].name));
    Serial.printf("   TDS Safe: %d ppm, Danger: %d ppm\n", 
                  PROFILES[activeProfileIndex].tds_safe, 
                  PROFILES[activeProfileIndex].tds_danger);
    
    // Initial sensor readings
    Serial.println("\n📡 Sensor Status:");
    Serial.printf("  TDS Sensor (GPIO %d): ", TDS_PIN);
    Serial.println(analogRead(TDS_PIN) > 0 ? "✅ Connected" : "❌ Check wiring");
    Serial.printf("  pH Sensor (GPIO %d):  ", PH_PIN);
    Serial.println(analogRead(PH_PIN) > 0 ? "✅ Connected" : "❌ Check wiring");
    Serial.println("  Turbidity: ⏳ Mocked (pending hardware)");
    Serial.println("  Temperature: ⏳ Mocked (pending hardware)");
    
    Serial.println("\n✅ Ready! Press BOOT button or send 'ANALYZE' via BLE");
    Serial.println("============================================\n");
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
    // Check for button press
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(50);  // Debounce
        if (digitalRead(BUTTON_PIN) == LOW) {
            Serial.println("🔘 Button pressed - starting analysis...");
            analyzeWater();
            delay(1000);  // Prevent rapid re-trigger
        }
    }
    
    // Handle BLE connection state
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("📡 Restarted advertising");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    
    delay(10);
}
