/**
 * Aqua-Mind Main Application
 * ==========================
 * Core application logic for the mobile PWA.
 */

class AquaMindApp {
    constructor() {
        // State
        this.connected = false;
        this.currentData = null;
        this.history = [];
        this.settings = this._loadSettings();

        // Modules
        this.bluetooth = null;
        this.gemini = new GeminiAnalyzer(this.settings.apiKey);

        // Initialize
        this._initBluetooth();
        this._initUI();
        this._initEventListeners();
        this._loadHistory();

        // Hide splash after animation
        setTimeout(() => this._hideSplash(), 2000);
    }

    // ============ Initialization ============

    _initBluetooth() {
        // Use simulated Bluetooth if in simulation mode or Web Bluetooth not supported
        if (this.settings.simulation || !('bluetooth' in navigator)) {
            console.log('🧪 Using simulated Bluetooth');
            this.bluetooth = new SimulatedBluetooth();

            if (this.settings.simulation) {
                this.bluetooth.setScenario(this.settings.simulation);
            }
        } else {
            this.bluetooth = new BluetoothManager();
        }

        // Set callbacks
        this.bluetooth.setConnectionCallback((connected) => {
            this._onConnectionChange(connected);
        });

        this.bluetooth.setDataCallback((data) => {
            this._onDataReceived(data);
        });
    }

    _initUI() {
        // Cache DOM elements
        this.elements = {
            // Splash
            splash: document.getElementById('splash'),
            app: document.getElementById('app'),

            // Connection
            btnConnect: document.getElementById('btn-connect'),
            connectionBanner: document.getElementById('connection-banner'),
            statusDot: document.querySelector('.status-dot'),

            // Score
            jalScore: document.getElementById('jal-score'),
            verdict: document.getElementById('verdict'),
            verdictMessage: document.getElementById('verdict-message'),
            scoreRingProgress: document.querySelector('.score-ring-progress'),
            btnTestWater: document.getElementById('btn-test-water'),

            // Readings
            readingTds: document.getElementById('reading-tds'),
            readingTurb: document.getElementById('reading-turb'),
            readingTemp: document.getElementById('reading-temp'),
            readingStability: document.getElementById('reading-stability'),
            barTds: document.getElementById('bar-tds'),
            barTurb: document.getElementById('bar-turb'),
            barTemp: document.getElementById('bar-temp'),
            barStability: document.getElementById('bar-stability'),

            // AI
            aiSection: document.getElementById('ai-section'),
            aiLoading: document.getElementById('ai-loading'),
            aiResult: document.getElementById('ai-result'),
            aiIcon: document.getElementById('ai-icon'),
            aiHeadline: document.getElementById('ai-headline'),
            aiExplanation: document.getElementById('ai-explanation'),
            aiActionText: document.getElementById('ai-action-text'),
            btnRefreshAi: document.getElementById('btn-refresh-ai'),

            // Profile
            profileName: document.getElementById('profile-name'),
            seasonalAlert: document.getElementById('seasonal-alert'),
            seasonalAlertText: document.getElementById('seasonal-alert-text'),

            // History
            historyList: document.getElementById('history-list'),

            // Settings
            btnSettings: document.getElementById('btn-settings'),
            settingsModal: document.getElementById('settings-modal'),
            settingProfile: document.getElementById('setting-profile'),
            settingApiKey: document.getElementById('setting-api-key'),
            settingAutoAi: document.getElementById('setting-auto-ai'),
            settingNotifications: document.getElementById('setting-notifications'),
            settingSimulation: document.getElementById('setting-simulation'),
            btnSaveSettings: document.getElementById('btn-save-settings'),

            // WiFi Settings
            settingWifiSsid: document.getElementById('setting-wifi-ssid'),
            settingWifiPassword: document.getElementById('setting-wifi-password'),
            btnSaveWifi: document.getElementById('btn-save-wifi'),

            // Toast
            toastContainer: document.getElementById('toast-container')
        };

        // Apply saved settings to UI
        this._applySettingsToUI();
    }

    _initEventListeners() {
        // Connection
        this.elements.btnConnect.addEventListener('click', () => this._handleConnect());
        this.elements.connectionBanner.addEventListener('click', () => this._handleConnect());

        // AI refresh
        this.elements.btnRefreshAi.addEventListener('click', () => this._runAiAnalysis());

        // Test Water button (triggers analysis on ESP32)
        if (this.elements.btnTestWater) {
            this.elements.btnTestWater.addEventListener('click', () => this._triggerAnalysis());
        }

        // Settings
        this.elements.btnSettings.addEventListener('click', () => this._openSettings());
        this.elements.settingsModal.querySelector('.modal-close').addEventListener('click',
            () => this._closeSettings());
        this.elements.settingsModal.querySelector('.modal-backdrop').addEventListener('click',
            () => this._closeSettings());
        this.elements.btnSaveSettings.addEventListener('click', () => this._saveSettings());

        // WiFi save button
        if (this.elements.btnSaveWifi) {
            this.elements.btnSaveWifi.addEventListener('click', () => this._sendWiFiCredentials());
        }

        // Bottom nav
        document.querySelectorAll('.nav-item').forEach(item => {
            item.addEventListener('click', (e) => {
                const view = e.currentTarget.dataset.view;
                this._switchView(view);
            });
        });

        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' && !this.elements.settingsModal.classList.contains('hidden')) {
                this._closeSettings();
            }
        });
    }

    _hideSplash() {
        this.elements.splash.classList.add('fade-out');
        this.elements.app.classList.remove('hidden');

        setTimeout(() => {
            this.elements.splash.style.display = 'none';
        }, 500);
    }

    // ============ Connection ============

    async _handleConnect() {
        if (this.connected) {
            // Disconnect
            this.bluetooth.disconnect();
            return;
        }

        try {
            // Update UI
            this.elements.statusDot.classList.add('connecting');
            this.elements.statusDot.classList.remove('disconnected');
            this._showToast('📱', 'Connecting to device...');

            // Connect
            await this.bluetooth.connect();

            // If using simulation, trigger a reading
            if (this.bluetooth instanceof SimulatedBluetooth) {
                setTimeout(() => this.bluetooth.triggerReading(), 1000);
            }

        } catch (error) {
            console.error('Connection failed:', error);
            this._showToast('❌', error.message || 'Connection failed');
            this.elements.statusDot.classList.remove('connecting');
            this.elements.statusDot.classList.add('disconnected');
        }
    }

    _onConnectionChange(connected) {
        this.connected = connected;

        // Update status dot
        this.elements.statusDot.classList.remove('connecting');
        if (connected) {
            this.elements.statusDot.classList.add('connected');
            this.elements.statusDot.classList.remove('disconnected');
            this.elements.connectionBanner.classList.add('hidden');

            // Show Test Water button when connected
            if (this.elements.btnTestWater) {
                this.elements.btnTestWater.classList.remove('hidden');
            }

            this._showToast('✅', 'Device connected!');
        } else {
            this.elements.statusDot.classList.remove('connected');
            this.elements.statusDot.classList.add('disconnected');
            this.elements.connectionBanner.classList.remove('hidden');

            // Hide Test Water button when disconnected
            if (this.elements.btnTestWater) {
                this.elements.btnTestWater.classList.add('hidden');
            }

            if (this.currentData) {
                this._showToast('📴', 'Device disconnected');
            }
        }
    }

    /**
     * Trigger water analysis on ESP32 via BLE command
     */
    async _triggerAnalysis() {
        if (!this.bluetooth.isConnected()) {
            this._showToast('❌', 'Not connected to device');
            return;
        }

        try {
            this._showToast('🔬', 'Starting analysis...');
            await this.bluetooth.send({ type: 'REQUEST_READING' });
        } catch (error) {
            console.error('Failed to trigger analysis:', error);
            this._showToast('❌', 'Failed to start analysis');
        }
    }

    /**
     * Send WiFi credentials to ESP32 via BLE
     */
    async _sendWiFiCredentials() {
        const ssid = this.elements.settingWifiSsid?.value?.trim();
        const password = this.elements.settingWifiPassword?.value;

        if (!ssid) {
            this._showToast('❌', 'Please enter WiFi name');
            return;
        }

        if (!password) {
            this._showToast('❌', 'Please enter WiFi password');
            return;
        }

        if (!this.bluetooth.isConnected()) {
            this._showToast('❌', 'Connect to device first');
            return;
        }

        try {
            // Send command: WIFI:ssid:password
            const command = `WIFI:${ssid}:${password}`;
            await this.bluetooth.sendCommand(command);

            this._showToast('📶', `Sent WiFi: ${ssid}`);

            // Clear the password field for security
            this.elements.settingWifiPassword.value = '';
        } catch (error) {
            console.error('Failed to send WiFi:', error);
            this._showToast('❌', 'Failed to send WiFi credentials');
        }
    }

    // ============ Data Handling ============

    _onDataReceived(data) {
        console.log('📊 Data received:', data);

        // ESP32 sends data directly without 'type' field
        // Check if this looks like analysis data
        if (data && (data.jal_score !== undefined || data.tds !== undefined || data.score !== undefined)) {
            try {
                // Normalize field names (ESP32 uses different names than simulated)
                const normalizedData = {
                    type: 'ANALYSIS_RESULT',
                    score: data.jal_score ?? data.score ?? 0,
                    verdict: data.verdict || 'UNKNOWN',
                    tds: data.tds ?? 0,
                    turb: data.turbidity ?? data.turb ?? 0,
                    temp: data.temperature ?? data.temp ?? 25,
                    stability: data.stability ?? 0,
                    ph: data.ph ?? 7,
                    message: this._getVerdictMessage(data.verdict),
                    profile: data.profile || 'Jabalpur',
                    alert: data.alert || ''
                };

                console.log('📊 Normalized data:', normalizedData);

                // Update location display in settings
                const locationEl = document.getElementById('detected-location');
                if (locationEl) {
                    locationEl.textContent = `📍 ${normalizedData.profile} (Auto-detected)`;
                }

                this.currentData = normalizedData;
                this._updateDisplay(normalizedData);
                this._addToHistory(normalizedData);

                // Show toast
                this._showToast('📊', `Jal-Score: ${normalizedData.score} - ${normalizedData.verdict}`);

                // Run AI analysis if enabled
                if (this.settings.autoAi && this.gemini.hasApiKey()) {
                    this._runAiAnalysis();
                }
            } catch (error) {
                console.error('Error processing data:', error);
            }
        } else if (data.type === 'ERROR') {
            this._showToast('❌', data.message);
        } else if (data.type === 'STATUS' || data.status) {
            this._showToast('ℹ️', data.status || 'Device ready');
        }
    }

    /**
     * Get verdict message
     */
    _getVerdictMessage(verdict) {
        const messages = {
            'SAFE': 'Water is safe for consumption',
            'ACCEPTABLE': 'Water quality is acceptable',
            'CAUTION': 'Use caution - consider treatment',
            'UNSAFE': 'Do NOT consume without treatment!'
        };
        return messages[verdict] || '';
    }

    _updateDisplay(data) {
        try {
            // Update score ring
            const score = Math.max(0, Math.min(100, data.score || 0));
            const circumference = 2 * Math.PI * 85;
            const offset = circumference - (score / 100) * circumference;

            if (this.elements.scoreRingProgress) {
                this.elements.scoreRingProgress.style.strokeDashoffset = offset;
            }
            if (this.elements.jalScore) {
                this.elements.jalScore.textContent = Math.round(score);
            }

            // Update verdict with color
            const verdict = (data.verdict || 'UNKNOWN').toUpperCase();
            if (this.elements.verdict) {
                this.elements.verdict.textContent = verdict;
                this.elements.verdict.className = 'verdict ' + verdict.toLowerCase();
            }
            if (this.elements.verdictMessage) {
                this.elements.verdictMessage.textContent = data.message || '';
            }

            // Update score ring color (SVG elements need setAttribute, not className)
            if (this.elements.scoreRingProgress) {
                this.elements.scoreRingProgress.setAttribute('class', 'score-ring-progress ' + verdict.toLowerCase());
            }

            // Update readings - direct DOM access for reliability
            this._safeUpdateReading('reading-tds', 'bar-tds', data.tds, 1000, 'ppm', 1);
            this._safeUpdateReading('reading-turb', 'bar-turb', data.turb, 20, 'NTU', 2);
            this._safeUpdateReading('reading-temp', 'bar-temp', data.temp, 50, '°C', 1);
            this._safeUpdateReading('reading-stability', 'bar-stability', data.stability, 100, '%', 1);

            // Update profile (with null check)
            if (data.profile && this.elements.profileName) {
                this.elements.profileName.textContent = data.profile;
            }

            // Update seasonal alert (with null check)
            if (this.elements.seasonalAlert) {
                if (data.alert) {
                    this.elements.seasonalAlert.classList.remove('hidden');
                    if (this.elements.seasonalAlertText) {
                        this.elements.seasonalAlertText.textContent = data.alert;
                    }
                } else {
                    this.elements.seasonalAlert.classList.add('hidden');
                }
            }

            console.log('✅ Display updated successfully');
        } catch (error) {
            console.error('Error updating display:', error);
        }
    }

    /**
     * Safely update a reading element
     */
    _safeUpdateReading(readingId, barId, value, max, unit, decimals) {
        try {
            const displayEl = document.getElementById(readingId);
            const barEl = document.getElementById(barId);

            if (displayEl) {
                const formatted = (value !== null && value !== undefined)
                    ? Number(value).toFixed(decimals)
                    : '--';
                displayEl.innerHTML = `${formatted} <small>${unit}</small>`;
            }

            if (barEl) {
                const percent = Math.min(100, Math.max(0, (value / max) * 100));
                barEl.style.width = `${percent}%`;
            }
        } catch (error) {
            console.error(`Error updating ${readingId}:`, error);
        }
    }

    _updateReading(param, value, max, unit) {
        const displayEl = this.elements[`reading${param.charAt(0).toUpperCase() + param.slice(1)}`];
        const barEl = this.elements[`bar${param.charAt(0).toUpperCase() + param.slice(1)}`];

        if (!displayEl || !barEl) return;

        // Format value
        const formatted = value !== null && value !== undefined
            ? Number(value).toFixed(param === 'turb' ? 2 : 1)
            : '--';

        displayEl.innerHTML = `${formatted} <small>${unit}</small>`;

        // Update bar
        const percent = Math.min(100, (value / max) * 100);
        barEl.style.width = `${percent}%`;

        // Color coding for bars
        barEl.classList.remove('warning', 'danger');
        if (param === 'tds' && value > 500) {
            barEl.classList.add(value > 900 ? 'danger' : 'warning');
        } else if (param === 'turb' && value > 1) {
            barEl.classList.add(value > 5 ? 'danger' : 'warning');
        } else if (param === 'stability' && value < 70) {
            barEl.classList.add(value < 50 ? 'danger' : 'warning');
        }
    }

    // ============ AI Analysis ============

    async _runAiAnalysis() {
        if (!this.currentData) {
            this._showToast('ℹ️', 'No data to analyze');
            return;
        }

        if (!this.gemini.hasApiKey()) {
            this._showToast('⚠️', 'Please set Gemini API key in settings');
            this._openSettings();
            return;
        }

        // Show loading
        this.elements.aiSection.classList.remove('hidden');
        this.elements.aiLoading.classList.remove('hidden');
        this.elements.aiResult.style.display = 'none';

        try {
            const analysis = await this.gemini.analyze(this.currentData);

            // Hide loading, show result
            this.elements.aiLoading.classList.add('hidden');
            this.elements.aiResult.style.display = 'flex';

            // Update AI result
            const statusIcons = {
                'Safe': '✅',
                'Caution': '⚠️',
                'Unsafe': '🚫',
                'Error': '❌'
            };

            this.elements.aiIcon.textContent = statusIcons[analysis.status] || 'ℹ️';
            this.elements.aiHeadline.textContent = analysis.headline;
            this.elements.aiExplanation.textContent = analysis.explanation;
            this.elements.aiActionText.textContent = analysis.action;

        } catch (error) {
            console.error('AI analysis failed:', error);
            this.elements.aiLoading.classList.add('hidden');
            this._showToast('❌', 'AI analysis failed');
        }
    }

    // ============ History ============

    _addToHistory(data) {
        const entry = {
            ...data,
            id: Date.now(),
            timestamp: new Date().toISOString()
        };

        this.history.unshift(entry);

        // Keep only last 50 entries
        if (this.history.length > 50) {
            this.history = this.history.slice(0, 50);
        }

        this._saveHistory();
        this._renderHistory();
    }

    _renderHistory() {
        if (this.history.length === 0) {
            this.elements.historyList.innerHTML =
                '<div class="history-empty">No tests yet. Connect device to start.</div>';
            return;
        }

        const html = this.history.slice(0, 10).map(entry => {
            const verdict = (entry.verdict || 'UNKNOWN').toLowerCase();
            const time = this._formatTime(entry.timestamp);
            const icon = { safe: '✅', caution: '⚠️', unsafe: '🚫', error: '❌' }[verdict] || '❓';

            return `
                <div class="history-item" data-id="${entry.id}">
                    <div class="history-icon ${verdict}">${icon}</div>
                    <div class="history-info">
                        <span class="history-score">Score: ${Math.round(entry.score)}</span>
                        <span class="history-time">${time}</span>
                    </div>
                    <span class="verdict history-verdict ${verdict}">${entry.verdict}</span>
                </div>
            `;
        }).join('');

        this.elements.historyList.innerHTML = html;
    }

    _formatTime(isoString) {
        const date = new Date(isoString);
        const now = new Date();
        const diff = now - date;

        if (diff < 60000) return 'Just now';
        if (diff < 3600000) return `${Math.floor(diff / 60000)}m ago`;
        if (diff < 86400000) return `${Math.floor(diff / 3600000)}h ago`;

        return date.toLocaleDateString('en-IN', {
            day: 'numeric',
            month: 'short',
            hour: '2-digit',
            minute: '2-digit'
        });
    }

    _saveHistory() {
        try {
            localStorage.setItem('aquamind_history', JSON.stringify(this.history));
        } catch (e) {
            console.warn('Failed to save history:', e);
        }
    }

    _loadHistory() {
        try {
            const saved = localStorage.getItem('aquamind_history');
            if (saved) {
                this.history = JSON.parse(saved);
                this._renderHistory();
            }
        } catch (e) {
            console.warn('Failed to load history:', e);
        }
    }

    // ============ Settings ============

    _loadSettings() {
        const defaults = {
            profile: 'DHANWANTRI_NAGAR',
            apiKey: '',
            autoAi: true,
            notifications: false,
            simulation: ''
        };

        try {
            const saved = localStorage.getItem('aquamind_settings');
            if (saved) {
                return { ...defaults, ...JSON.parse(saved) };
            }
        } catch (e) {
            console.warn('Failed to load settings:', e);
        }

        return defaults;
    }

    _applySettingsToUI() {
        // Profile is now auto-detected by ESP32, no manual selector
        if (this.elements.settingApiKey) {
            this.elements.settingApiKey.value = this.settings.apiKey || '';
        }
        if (this.elements.settingAutoAi) {
            this.elements.settingAutoAi.checked = this.settings.autoAi;
        }
        if (this.elements.settingNotifications) {
            this.elements.settingNotifications.checked = this.settings.notifications;
        }
        if (this.elements.settingSimulation) {
            this.elements.settingSimulation.value = this.settings.simulation || '';
        }
    }

    _openSettings() {
        this.elements.settingsModal.classList.remove('hidden');
    }

    _closeSettings() {
        this.elements.settingsModal.classList.add('hidden');
    }

    _saveSettings() {
        // Collect settings
        this.settings.profile = this.elements.settingProfile.value;
        this.settings.apiKey = this.elements.settingApiKey.value;
        this.settings.autoAi = this.elements.settingAutoAi.checked;
        this.settings.notifications = this.elements.settingNotifications.checked;
        this.settings.simulation = this.elements.settingSimulation.value;

        // Save to localStorage
        try {
            localStorage.setItem('aquamind_settings', JSON.stringify(this.settings));
        } catch (e) {
            console.warn('Failed to save settings:', e);
        }

        // Apply settings
        this.gemini.setApiKey(this.settings.apiKey);

        // Handle simulation mode change
        if (this.settings.simulation) {
            if (!(this.bluetooth instanceof SimulatedBluetooth)) {
                this.bluetooth.disconnect();
                this.bluetooth = new SimulatedBluetooth();
                this.bluetooth.setConnectionCallback((c) => this._onConnectionChange(c));
                this.bluetooth.setDataCallback((d) => this._onDataReceived(d));
            }
            this.bluetooth.setScenario(this.settings.simulation);
        }

        this._closeSettings();
        this._showToast('✅', 'Settings saved');
    }

    // ============ Navigation ============

    _switchView(view) {
        // Update nav
        document.querySelectorAll('.nav-item').forEach(item => {
            item.classList.toggle('active', item.dataset.view === view);
        });

        // Handle view switching (simplified for now)
        if (view === 'settings') {
            this._openSettings();
        }
    }

    // ============ Utilities ============

    _showToast(icon, message, duration = 3000) {
        const toast = document.createElement('div');
        toast.className = 'toast';
        toast.innerHTML = `
            <span class="toast-icon">${icon}</span>
            <span class="toast-message">${message}</span>
        `;

        this.elements.toastContainer.appendChild(toast);

        // Remove after duration
        setTimeout(() => {
            toast.classList.add('fade-out');
            setTimeout(() => toast.remove(), 300);
        }, duration);
    }
}


// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.app = new AquaMindApp();
});
