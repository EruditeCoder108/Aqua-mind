/**
 * Aqua-Mind Web Bluetooth Module
 * ==============================
 * Handles Bluetooth communication with the Raspberry Pi device.
 * Uses Web Bluetooth API for SPP (Serial Port Profile) emulation.
 */

class BluetoothManager {
    // UUIDs must match ESP32 code exactly
    static SERVICE_UUID = '12345678-1234-1234-1234-123456789abc';
    static CHAR_DATA_UUID = '12345678-1234-1234-1234-123456789abd';
    static CHAR_COMMAND_UUID = '12345678-1234-1234-1234-123456789abe';

    constructor() {
        this.device = null;
        this.server = null;
        this.service = null;
        this.dataChar = null;
        this.commandChar = null;
        this.connected = false;
        this.onDataReceived = null;
        this.onConnectionChange = null;
        this.buffer = '';

        // Check if Web Bluetooth is supported
        this.supported = 'bluetooth' in navigator;

        if (!this.supported) {
            console.warn('Web Bluetooth not supported in this browser');
        }
    }

    // ... (keep matches)

    async connect() {
        if (!this.supported) {
            throw new Error('Web Bluetooth not supported');
        }

        try {
            console.log('🔍 Scanning for Aqua-Mind device...');

            // Request device with name filter and optional service
            this.device = await navigator.bluetooth.requestDevice({
                filters: [
                    { name: 'AquaMind-ESP32' },
                    { namePrefix: 'Aqua' },
                    { namePrefix: 'ESP32' }
                ],
                optionalServices: [BluetoothManager.SERVICE_UUID]
            });

            console.log(`📱 Device found: ${this.device.name}`);

            this.device.addEventListener('gattserverdisconnected', () => {
                this._handleDisconnect();
            });

            console.log('🔗 Connecting to GATT server...');
            this.server = await this.device.gatt.connect();

            console.log('📡 Getting service...');
            this.service = await this.server.getPrimaryService(BluetoothManager.SERVICE_UUID);

            // Get Data Characteristic (Notify)
            console.log('📝 Getting data characteristic...');
            this.dataChar = await this.service.getCharacteristic(BluetoothManager.CHAR_DATA_UUID);

            // Get Command Characteristic (Write)
            console.log('📝 Getting command characteristic...');
            this.commandChar = await this.service.getCharacteristic(BluetoothManager.CHAR_COMMAND_UUID);

            // Start notifications
            await this.dataChar.startNotifications();
            this.dataChar.addEventListener('characteristicvaluechanged',
                (event) => this._handleData(event));

            this.connected = true;
            console.log('✅ Connected to Aqua-Mind device!');

            if (this.onConnectionChange) {
                this.onConnectionChange(true);
            }

            return true;
        } catch (error) {
            console.error('❌ Bluetooth connection failed:', error);
            this.connected = false;
            if (this.onConnectionChange) this.onConnectionChange(false);
            throw error;
        }
    }

    _handleDisconnect() {
        console.log('📴 Bluetooth disconnected');
        this.connected = false;
        this.server = null;
        this.service = null;
        this.dataChar = null;
        this.commandChar = null;

        if (this.onConnectionChange) {
            this.onConnectionChange(false);
        }
    }

    // ... (keep matches)

    async send(command) {
        if (!this.isConnected() || !this.commandChar) {
            throw new Error('Not connected');
        }

        // ESP32 expects raw string commands like "ANALYZE" for simple logic
        // But invalid commands might be ignored. Let's send what the ESP32 expects.
        // The Arduino code looks for "ANALYZE" or "STATUS" strings.
        // If the inputs are JSON objects (like {type: 'REQUEST_READING'}), we should convert logic.

        let textToSend = '';
        if (typeof command === 'object' && command.type === 'REQUEST_READING') {
            textToSend = 'ANALYZE';
        } else if (typeof command === 'string') {
            textToSend = command;
        } else {
            textToSend = JSON.stringify(command);
        }

        const encoder = new TextEncoder();
        const data = encoder.encode(textToSend);

        await this.commandChar.writeValue(data);
        console.log('📤 Sent command:', textToSend);
    }

    /**
     * Set callback for received data
     */
    setDataCallback(callback) {
        this.onDataReceived = callback;
    }

    /**
     * Set callback for connection changes
     */
    setConnectionCallback(callback) {
        this.onConnectionChange = callback;
    }
}


/**
 * Simulated Bluetooth for testing without real hardware
 */
class SimulatedBluetooth {
    constructor() {
        this.connected = false;
        this.onDataReceived = null;
        this.onConnectionChange = null;
        this.simulationInterval = null;
        this.scenario = 'tap_water';
    }

    isSupported() {
        return true;
    }

    isConnected() {
        return this.connected;
    }

    setScenario(scenario) {
        this.scenario = scenario;
    }

    async connect() {
        console.log('🧪 Connecting simulated Bluetooth...');

        // Simulate connection delay
        await new Promise(resolve => setTimeout(resolve, 1000));

        this.connected = true;
        console.log('✅ Simulated Bluetooth connected');

        if (this.onConnectionChange) {
            this.onConnectionChange(true);
        }

        return true;
    }

    disconnect() {
        this.connected = false;

        if (this.simulationInterval) {
            clearInterval(this.simulationInterval);
            this.simulationInterval = null;
        }

        if (this.onConnectionChange) {
            this.onConnectionChange(false);
        }
    }

    /**
     * Generate simulated reading
     */
    _generateReading() {
        const scenarios = {
            clean_water: {
                tds: 120 + Math.random() * 50,
                turb: 0.3 + Math.random() * 0.4,
                temp: 24 + Math.random() * 2,
                stability: 92 + Math.random() * 6,
                score: 88 + Math.random() * 10,
                verdict: 'SAFE'
            },
            tap_water: {
                tds: 320 + Math.random() * 80,
                turb: 1.5 + Math.random() * 1,
                temp: 27 + Math.random() * 3,
                stability: 82 + Math.random() * 10,
                score: 70 + Math.random() * 15,
                verdict: 'CAUTION'
            },
            dirty_water: {
                tds: 550 + Math.random() * 150,
                turb: 6 + Math.random() * 3,
                temp: 29 + Math.random() * 4,
                stability: 65 + Math.random() * 15,
                score: 35 + Math.random() * 20,
                verdict: 'UNSAFE'
            },
            contaminated: {
                tds: 850 + Math.random() * 200,
                turb: 12 + Math.random() * 5,
                temp: 31 + Math.random() * 3,
                stability: 50 + Math.random() * 15,
                score: 15 + Math.random() * 15,
                verdict: 'UNSAFE'
            },
            sensor_error: {
                tds: 400 + Math.random() * 300,
                turb: 4 + Math.random() * 5,
                temp: 25 + Math.random() * 10,
                stability: 25 + Math.random() * 20,
                score: 40 + Math.random() * 20,
                verdict: 'ERROR'
            }
        };

        const data = scenarios[this.scenario] || scenarios.tap_water;

        return {
            type: 'ANALYSIS_RESULT',
            ts: new Date().toISOString(),
            tds: Math.round(data.tds * 10) / 10,
            turb: Math.round(data.turb * 100) / 100,
            temp: Math.round(data.temp * 10) / 10,
            stability: Math.round(data.stability * 10) / 10,
            score: Math.round(data.score * 10) / 10,
            verdict: data.verdict,
            message: this._getVerdictMessage(data.verdict),
            profile: 'Jabalpur, Madhya Pradesh',
            alert: '',
            sim: true
        };
    }

    _getVerdictMessage(verdict) {
        const messages = {
            SAFE: 'Water appears safe for consumption',
            CAUTION: 'Water quality marginal - treatment recommended',
            UNSAFE: 'Water unsafe - do not consume without treatment',
            ERROR: 'Sensor error - clean probe and retry'
        };
        return messages[verdict] || messages.ERROR;
    }

    /**
     * Trigger a simulated reading
     */
    triggerReading() {
        if (!this.connected) return;

        const data = this._generateReading();
        console.log('🧪 Simulated reading:', data);

        if (this.onDataReceived) {
            this.onDataReceived(data);
        }
    }

    /**
     * Start continuous simulation
     */
    startContinuous(intervalMs = 5000) {
        if (this.simulationInterval) {
            clearInterval(this.simulationInterval);
        }

        this.simulationInterval = setInterval(() => {
            this.triggerReading();
        }, intervalMs);
    }

    async send(command) {
        console.log('🧪 Simulated send:', command);

        // If it's a request for reading, trigger one
        if (command.type === 'REQUEST_READING') {
            setTimeout(() => this.triggerReading(), 500);
        }
    }

    setDataCallback(callback) {
        this.onDataReceived = callback;
    }

    setConnectionCallback(callback) {
        this.onConnectionChange = callback;
    }
}


// Export for use in app.js
window.BluetoothManager = BluetoothManager;
window.SimulatedBluetooth = SimulatedBluetooth;
