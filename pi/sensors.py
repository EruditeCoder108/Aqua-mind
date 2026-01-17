"""
Aqua-Mind Sensor Drivers
========================
Hardware abstraction layer for TDS, Turbidity, and Temperature sensors.
Includes SIMULATION MODE for testing without actual hardware.

Hardware Requirements:
- MCP3008 ADC (SPI) for analog sensors
- DS18B20 for temperature
- Calibration button on GPIO 17
"""

import time
import random
import os
import json
from pathlib import Path

# Detect if running on Raspberry Pi
IS_RASPBERRY_PI = os.path.exists('/proc/device-tree/model')

if IS_RASPBERRY_PI:
    try:
        import spidev
        import RPi.GPIO as GPIO
        HARDWARE_AVAILABLE = True
    except ImportError:
        HARDWARE_AVAILABLE = False
        print("⚠️  Hardware libraries not found. Running in SIMULATION mode.")
else:
    HARDWARE_AVAILABLE = False
    print("🖥️  Not running on Raspberry Pi. SIMULATION mode enabled.")


class SimulatedSensors:
    """
    Simulates sensor readings for testing without hardware.
    Generates realistic noisy data with configurable scenarios.
    """
    
    SCENARIOS = {
        "clean_water": {
            "tds_base": 150,
            "tds_noise": 10,
            "turb_base": 0.5,
            "turb_noise": 0.2,
            "temp_base": 25,
            "temp_noise": 0.5,
            "stability": 0.95
        },
        "tap_water": {
            "tds_base": 350,
            "tds_noise": 25,
            "turb_base": 1.5,
            "turb_noise": 0.5,
            "temp_base": 28,
            "temp_noise": 1,
            "stability": 0.85
        },
        "dirty_water": {
            "tds_base": 650,
            "tds_noise": 50,
            "turb_base": 8,
            "turb_noise": 2,
            "temp_base": 30,
            "temp_noise": 2,
            "stability": 0.70
        },
        "contaminated": {
            "tds_base": 900,
            "tds_noise": 100,
            "turb_base": 15,
            "turb_noise": 5,
            "temp_base": 32,
            "temp_noise": 3,
            "stability": 0.50
        },
        "sensor_error": {
            "tds_base": 500,
            "tds_noise": 200,
            "turb_base": 5,
            "turb_noise": 4,
            "temp_base": 25,
            "temp_noise": 10,
            "stability": 0.20
        }
    }
    
    def __init__(self, scenario="tap_water"):
        self.set_scenario(scenario)
        self._button_pressed = False
        print(f"🧪 Simulation initialized with scenario: {scenario}")
    
    def set_scenario(self, scenario):
        """Change the simulation scenario."""
        if scenario not in self.SCENARIOS:
            print(f"⚠️  Unknown scenario '{scenario}'. Using 'tap_water'.")
            scenario = "tap_water"
        self.scenario = scenario
        self._params = self.SCENARIOS[scenario]
    
    def read_tds_raw(self):
        """Simulate raw TDS ADC reading (0-1023)."""
        base = self._params["tds_base"]
        noise = self._params["tds_noise"]
        stability = self._params["stability"]
        
        # Add instability factor
        drift = random.gauss(0, noise * (1 - stability))
        value = base + random.gauss(0, noise) + drift
        
        # Convert to ADC range (0-1023), assuming 0-1000ppm maps to 0-1023
        adc_value = int((value / 1000) * 1023)
        return max(0, min(1023, adc_value))
    
    def read_turbidity_raw(self):
        """Simulate raw Turbidity ADC reading (0-1023)."""
        base = self._params["turb_base"]
        noise = self._params["turb_noise"]
        stability = self._params["stability"]
        
        drift = random.gauss(0, noise * (1 - stability))
        value = base + random.gauss(0, noise) + drift
        
        # Convert to ADC range (0-1023), assuming 0-20 NTU maps to 0-1023
        adc_value = int((value / 20) * 1023)
        return max(0, min(1023, adc_value))
    
    def read_temperature(self):
        """Simulate temperature reading in Celsius."""
        base = self._params["temp_base"]
        noise = self._params["temp_noise"]
        return round(base + random.gauss(0, noise), 1)
    
    def is_button_pressed(self):
        """Simulate button press (random for demo)."""
        # In real use, you'd set this programmatically for testing
        return self._button_pressed
    
    def simulate_button_press(self):
        """Trigger a simulated button press."""
        self._button_pressed = True
    
    def clear_button(self):
        """Clear button state."""
        self._button_pressed = False


class HardwareSensors:
    """
    Real hardware sensor drivers for Raspberry Pi.
    Uses MCP3008 ADC via SPI for analog sensors.
    """
    
    # GPIO Pin definitions
    BUTTON_PIN = 17
    DS18B20_PIN = 4
    
    # MCP3008 channel assignments
    TDS_CHANNEL = 0
    TURBIDITY_CHANNEL = 1
    
    # Calibration file
    CALIBRATION_FILE = Path(__file__).parent / "calibration.json"
    
    def __init__(self):
        if not HARDWARE_AVAILABLE:
            raise RuntimeError("Hardware libraries not available!")
        
        # Initialize SPI for MCP3008
        self.spi = spidev.SpiDev()
        self.spi.open(0, 0)  # Bus 0, Device 0 (CE0)
        self.spi.max_speed_hz = 1350000
        
        # Initialize GPIO
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(self.BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        
        # Load calibration
        self.calibration = self._load_calibration()
        
        # Initialize DS18B20
        self._init_ds18b20()
        
        print("✅ Hardware sensors initialized")
    
    def _load_calibration(self):
        """Load calibration offsets from file."""
        default = {
            "tds_offset": 0,
            "tds_scale": 1.0,
            "turb_offset": 0,
            "turb_scale": 1.0
        }
        
        if self.CALIBRATION_FILE.exists():
            try:
                with open(self.CALIBRATION_FILE, 'r') as f:
                    return json.load(f)
            except:
                pass
        
        return default
    
    def _init_ds18b20(self):
        """Initialize DS18B20 1-Wire temperature sensor."""
        # Enable 1-Wire interface
        base_dir = '/sys/bus/w1/devices/'
        try:
            device_folders = [f for f in os.listdir(base_dir) if f.startswith('28-')]
            if device_folders:
                self.ds18b20_path = base_dir + device_folders[0] + '/w1_slave'
            else:
                self.ds18b20_path = None
                print("⚠️  DS18B20 not found. Temperature will be estimated.")
        except:
            self.ds18b20_path = None
    
    def _read_adc(self, channel):
        """Read raw value from MCP3008 ADC channel (0-7)."""
        if channel < 0 or channel > 7:
            return 0
        
        # MCP3008 protocol: Start bit, single-ended, channel
        adc = self.spi.xfer2([1, (8 + channel) << 4, 0])
        value = ((adc[1] & 3) << 8) + adc[2]
        return value
    
    def read_tds_raw(self):
        """Read raw TDS ADC value."""
        return self._read_adc(self.TDS_CHANNEL)
    
    def read_turbidity_raw(self):
        """Read raw Turbidity ADC value (already voltage-divided)."""
        return self._read_adc(self.TURBIDITY_CHANNEL)
    
    def read_temperature(self):
        """Read temperature from DS18B20 sensor."""
        if not self.ds18b20_path:
            return 25.0  # Default fallback
        
        try:
            with open(self.ds18b20_path, 'r') as f:
                lines = f.readlines()
            
            if lines[0].strip()[-3:] == 'YES':
                equals_pos = lines[1].find('t=')
                if equals_pos != -1:
                    temp_string = lines[1][equals_pos + 2:]
                    temp_c = float(temp_string) / 1000.0
                    return round(temp_c, 1)
        except:
            pass
        
        return 25.0
    
    def is_button_pressed(self):
        """Check if calibration button is pressed."""
        return GPIO.input(self.BUTTON_PIN) == GPIO.LOW
    
    def cleanup(self):
        """Clean up GPIO on exit."""
        GPIO.cleanup()


class SensorManager:
    """
    High-level sensor manager with automatic mode detection.
    Provides unified interface regardless of hardware availability.
    """
    
    # Conversion constants (calibrated for common sensors)
    TDS_PPM_PER_ADC = 1000 / 1023  # 0-1000 ppm range
    TURB_NTU_PER_ADC = 20 / 1023   # 0-20 NTU range
    
    def __init__(self, simulation_scenario=None):
        """
        Initialize sensor manager.
        
        Args:
            simulation_scenario: Force simulation mode with specific scenario.
                                Options: clean_water, tap_water, dirty_water, 
                                         contaminated, sensor_error
        """
        self.simulation_mode = not HARDWARE_AVAILABLE or simulation_scenario is not None
        
        if self.simulation_mode:
            scenario = simulation_scenario or "tap_water"
            self._driver = SimulatedSensors(scenario)
        else:
            self._driver = HardwareSensors()
        
        # Load calibration offsets
        self.tds_offset = 0
        self.turb_offset = 0
    
    def set_scenario(self, scenario):
        """Change simulation scenario (only works in simulation mode)."""
        if self.simulation_mode:
            self._driver.set_scenario(scenario)
        else:
            print("⚠️  Cannot change scenario in hardware mode.")
    
    def read_tds_raw(self):
        """Get raw TDS ADC reading."""
        return self._driver.read_tds_raw()
    
    def read_tds_ppm(self):
        """Get TDS value in ppm (parts per million)."""
        raw = self.read_tds_raw()
        ppm = raw * self.TDS_PPM_PER_ADC + self.tds_offset
        return max(0, round(ppm, 1))
    
    def read_turbidity_raw(self):
        """Get raw Turbidity ADC reading."""
        return self._driver.read_turbidity_raw()
    
    def read_turbidity_ntu(self):
        """Get Turbidity value in NTU (Nephelometric Turbidity Units)."""
        raw = self.read_turbidity_raw()
        ntu = raw * self.TURB_NTU_PER_ADC + self.turb_offset
        return max(0, round(ntu, 2))
    
    def read_temperature(self):
        """Get temperature in Celsius."""
        return self._driver.read_temperature()
    
    def is_button_pressed(self):
        """Check if calibration button is pressed."""
        return self._driver.is_button_pressed()
    
    def read_all(self):
        """Read all sensors at once."""
        return {
            "tds_raw": self.read_tds_raw(),
            "tds_ppm": self.read_tds_ppm(),
            "turbidity_raw": self.read_turbidity_raw(),
            "turbidity_ntu": self.read_turbidity_ntu(),
            "temperature_c": self.read_temperature(),
            "timestamp": time.time(),
            "simulation_mode": self.simulation_mode
        }
    
    def calibrate_tds(self, known_ppm):
        """
        Calibrate TDS sensor with known solution.
        
        Args:
            known_ppm: Known TDS value of calibration solution
        """
        # Take average of 10 readings
        readings = [self.read_tds_raw() for _ in range(10)]
        avg_raw = sum(readings) / len(readings)
        measured_ppm = avg_raw * self.TDS_PPM_PER_ADC
        
        self.tds_offset = known_ppm - measured_ppm
        print(f"📐 TDS Calibrated: offset = {self.tds_offset:.1f} ppm")
        return self.tds_offset
    
    def calibrate_turbidity(self, known_ntu):
        """
        Calibrate Turbidity sensor with known solution.
        
        Args:
            known_ntu: Known turbidity value (0 for clear water)
        """
        readings = [self.read_turbidity_raw() for _ in range(10)]
        avg_raw = sum(readings) / len(readings)
        measured_ntu = avg_raw * self.TURB_NTU_PER_ADC
        
        self.turb_offset = known_ntu - measured_ntu
        print(f"📐 Turbidity Calibrated: offset = {self.turb_offset:.2f} NTU")
        return self.turb_offset
    
    def cleanup(self):
        """Clean up resources."""
        if hasattr(self._driver, 'cleanup'):
            self._driver.cleanup()


# Quick test when run directly
if __name__ == "__main__":
    print("=" * 50)
    print("AQUA-MIND SENSOR TEST")
    print("=" * 50)
    
    # Test in simulation mode
    sensors = SensorManager(simulation_scenario="tap_water")
    
    print("\n📊 Testing different scenarios:\n")
    
    for scenario in ["clean_water", "tap_water", "dirty_water", "contaminated"]:
        sensors.set_scenario(scenario)
        time.sleep(0.1)
        
        readings = sensors.read_all()
        print(f"Scenario: {scenario:15} | TDS: {readings['tds_ppm']:6.1f} ppm | "
              f"Turb: {readings['turbidity_ntu']:5.2f} NTU | "
              f"Temp: {readings['temperature_c']:5.1f}°C")
    
    print("\n✅ Sensor test complete!")
