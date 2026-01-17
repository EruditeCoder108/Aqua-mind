/*
 * Aqua-Mind ESP32 - Water Quality Intelligence
 * =============================================
 * 
 * Hardware:
 * - ESP32 DevKit (30-pin)
 * - TDS Sensor V1.0 (GPIO 34)
 * - pH Sensor Module (GPIO 35)
 * - WiFi for Location/Weather
 * - Turbidity: MOCKED (pending hardware)
 * - Temperature: MOCKED (pending hardware)
 * 
 * Features:
 * - 5-Pillar Trust Engine
 * - Tri-Check burst sampling
 * - Jal-Score calculation
 * - BLE communication to mobile app
 * - WiFi-based location & weather
 * 
 * Author: Aqua-Mind Team
 * License: MIT
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>  // For saving WiFi credentials
// Using simple String-based JSON parsing (no ArduinoJson needed)

// ============================================
// WIFI CONFIGURATION
// ============================================
// Default WiFi Credentials (can be changed via BLE)
String wifiSSID = "Airtel_Sam";
String wifiPassword = "Samj@777";

bool wifiConnected = false;
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
String currentSeason = "normal";
float currentWeatherTemp = 25.0;
bool isRaining = false;

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
#define WEIGHT_TDS          0.35
#define WEIGHT_PH           0.30  // New! Using pH instead of just turbidity
#define WEIGHT_TURBIDITY    0.20
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
 */
float readPH() {
    int raw = readPHRaw();
    float voltage = (raw / 4095.0) * VREF * 1000.0; // mV
    
    // pH calculation (typical for ADS-based pH sensor)
    // Neutral pH 7.0 is around 1500mV, slope is about 59mV per pH unit
    float ph = 7.0 + ((1500.0 - voltage) / 59.16);
    ph += PH_OFFSET;
    
    // Clamp to valid range
    return constrain(ph, 0.0f, 14.0f);
}

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
    
    // Stability score: 100% = no variance, 0% = high variance
    // Using coefficient of variation (CV)
    float cv = (mean > 0) ? (stdDev / mean) * 100.0 : 0;
    *stability = max(0.0f, 100.0f - cv * 10.0f);  // Scale CV to 0-100
    
    Serial.printf("  📊 Tri-Check Result: Mean=%.2f, Stability=%.1f%%\n", mean, *stability);
    
    return mean;
}

// ============================================
// JAL-SCORE CALCULATION
// ============================================

/**
 * Calculate Jal-Score (0-100) based on all parameters
 * Uses weighted formula with regional thresholds
 */
int calculateJalScore(float tds, float ph, float turbidity, float stability) {
    // TDS Score (0-100, higher TDS = lower score)
    float tdsScore;
    if (tds <= TDS_SAFE_LIMIT) {
        tdsScore = 100.0;
    } else if (tds >= TDS_DANGER_LIMIT) {
        tdsScore = 0.0;
    } else {
        tdsScore = 100.0 - ((tds - TDS_SAFE_LIMIT) / (TDS_DANGER_LIMIT - TDS_SAFE_LIMIT)) * 100.0;
    }
    
    // pH Score (0-100, best at 7.0)
    float phScore;
    if (ph >= PH_SAFE_MIN && ph <= PH_SAFE_MAX) {
        // Within safe range, score based on distance from 7.0
        float distFrom7 = abs(ph - 7.0);
        phScore = 100.0 - (distFrom7 * 20.0);  // Lose 20 points per unit from 7
    } else if (ph < PH_SAFE_MIN) {
        float penalty = ((float)PH_SAFE_MIN - ph) * 30.0f;
        phScore = (penalty > 50.0f) ? 0.0f : (50.0f - penalty);
    } else {
        float penalty = (ph - (float)PH_SAFE_MAX) * 30.0f;
        phScore = (penalty > 50.0f) ? 0.0f : (50.0f - penalty);
    }
    
    // Turbidity Score (0-100, lower is better)
    float turbScore;
    if (turbidity <= 1.0) {
        turbScore = 100.0;
    } else if (turbidity >= TURBIDITY_LIMIT) {
        turbScore = 0.0;
    } else {
        turbScore = 100.0 - ((turbidity - 1.0) / (TURBIDITY_LIMIT - 1.0)) * 100.0;
    }
    
    // Weighted final score
    float jalScore = (tdsScore * WEIGHT_TDS) +
                     (phScore * WEIGHT_PH) +
                     (turbScore * WEIGHT_TURBIDITY) +
                     (stability * WEIGHT_STABILITY);
    
    // Apply stability penalty if readings are unstable
    if (stability < 70) {
        jalScore *= 0.9;  // 10% penalty for unstable readings
    }
    
    Serial.printf("  📈 Score Components: TDS=%.1f, pH=%.1f, Turb=%.1f, Stab=%.1f\n",
                  tdsScore, phScore, turbScore, stability);
    
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
    
    // Overall stability (average of TDS and pH stability)
    lastStability = (tdsStability + phStability) / 2.0;
    
    // Calculate Jal-Score
    Serial.println("\n  🎯 Calculating Jal-Score...");
    lastJalScore = calculateJalScore(lastTDS, lastPH, lastTurbidity, lastStability);
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
 */
void sendAnalysisViaBLE() {
    // Create JSON string
    String json = "{";
    json += "\"tds\":" + String(lastTDS, 1) + ",";
    json += "\"ph\":" + String(lastPH, 2) + ",";
    json += "\"turbidity\":" + String(lastTurbidity, 2) + ",";
    json += "\"temperature\":" + String(lastTemperature, 1) + ",";
    json += "\"stability\":" + String(lastStability, 1) + ",";
    json += "\"jal_score\":" + String(lastJalScore) + ",";
    json += "\"verdict\":\"" + lastVerdict + "\",";
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
 */
void fetchWeather() {
    if (!wifiConnected || detectedLat == 0) return;
    
    Serial.println("🌤️ Fetching weather...");
    
    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(detectedLat, 2) + 
                 "&longitude=" + String(detectedLon, 2) + "&current_weather=true";
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        // Extract temperature and weathercode
        int tempIdx = payload.indexOf("\"temperature\":");
        int codeIdx = payload.indexOf("\"weathercode\":");
        
        if (tempIdx > 0) {
            currentWeatherTemp = payload.substring(tempIdx + 14, payload.indexOf(",", tempIdx)).toFloat();
        }
        
        if (codeIdx > 0) {
            int weatherCode = payload.substring(codeIdx + 14, payload.indexOf(",", codeIdx)).toInt();
            // Weather codes 51-99 indicate precipitation
            isRaining = (weatherCode >= 51 && weatherCode <= 99);
        }
        
        Serial.printf("   Weather: %.1f°C, %s\n", currentWeatherTemp, isRaining ? "🌧️ Raining" : "☀️ Clear");
        
        // Determine season
        // Get current month (rough estimation from compile time)
        // In real app, you'd use NTP time
        int month = 1; // Default to January
        
        // Monsoon detection: July-September + raining
        if (isRaining && month >= 6 && month <= 9) {
            currentSeason = "monsoon";
            Serial.println("   🌧️ Monsoon season detected!");
        } else if (currentWeatherTemp > 35 && month >= 3 && month <= 5) {
            currentSeason = "summer";
            Serial.println("   ☀️ Summer season detected!");
        } else {
            currentSeason = "normal";
        }
    } else {
        Serial.printf("⚠️ Weather fetch failed: %d\n", httpCode);
    }
    
    http.end();
}

// ============================================
// SETUP
// ============================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n");
    Serial.println("============================================");
    Serial.println("  🌊 AQUA-MIND Water Quality Intelligence");
    Serial.println("  📟 ESP32 Edition v2.0 (WiFi Enabled)");
    Serial.println("============================================");
    
    // Configure ADC
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_11db);  // Full 0-3.3V range
    
    // Configure button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize BLE
    setupBLE();
    
    // Load saved WiFi credentials (if any)
    loadSavedWiFi();
    
    // Connect to WiFi for location/weather
    connectWiFi();
    
    // Get location from IP (if WiFi connected)
    if (wifiConnected) {
        fetchLocation();
        fetchWeather();
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
