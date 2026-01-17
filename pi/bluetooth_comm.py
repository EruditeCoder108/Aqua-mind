"""
Aqua-Mind Bluetooth Communication
=================================
Handles Bluetooth Serial communication with mobile devices.
Sends water analysis results as JSON for the mobile app.
"""

import json
import time
import threading
from typing import Optional, Callable
import os

# Check if running on Raspberry Pi
IS_RASPBERRY_PI = os.path.exists('/proc/device-tree/model')

if IS_RASPBERRY_PI:
    try:
        import serial
        SERIAL_AVAILABLE = True
    except ImportError:
        SERIAL_AVAILABLE = False
        print("⚠️  PySerial not installed. Bluetooth disabled.")
else:
    SERIAL_AVAILABLE = False
    print("🖥️  Not on Pi. Using simulated Bluetooth.")


class SimulatedBluetooth:
    """
    Simulates Bluetooth communication for testing.
    Prints to console instead of actual transmission.
    """
    
    def __init__(self):
        self.connected = False
        self.last_sent = None
        print("📱 Simulated Bluetooth initialized")
    
    def connect(self) -> bool:
        """Simulate connection."""
        print("📱 [SIM] Bluetooth device connected")
        self.connected = True
        return True
    
    def disconnect(self):
        """Simulate disconnection."""
        print("📱 [SIM] Bluetooth device disconnected")
        self.connected = False
    
    def is_connected(self) -> bool:
        """Check connection status."""
        return self.connected
    
    def send(self, data: dict) -> bool:
        """
        Simulate sending data.
        
        Args:
            data: Dictionary to send as JSON
            
        Returns:
            True if successful
        """
        if not self.connected:
            print("❌ [SIM] Cannot send - not connected")
            return False
        
        json_str = json.dumps(data, indent=2)
        self.last_sent = data
        
        print("\n📤 [SIM] Sending via Bluetooth:")
        print("-" * 40)
        print(json_str)
        print("-" * 40)
        
        return True
    
    def receive(self, timeout: float = 5.0) -> Optional[dict]:
        """
        Simulate receiving data.
        
        Args:
            timeout: Timeout in seconds
            
        Returns:
            Received data or None
        """
        # In simulation, we can inject test commands
        return None


class BluetoothSerial:
    """
    Raspberry Pi Bluetooth Serial communication.
    Uses RFCOMM serial port for SPP (Serial Port Profile).
    """
    
    DEFAULT_PORT = "/dev/rfcomm0"
    BAUD_RATE = 9600
    
    def __init__(self, port: str = None):
        """
        Initialize Bluetooth serial.
        
        Args:
            port: Serial port path (default: /dev/rfcomm0)
        """
        if not SERIAL_AVAILABLE:
            raise RuntimeError("PySerial not available!")
        
        self.port = port or self.DEFAULT_PORT
        self.serial: Optional[serial.Serial] = None
        self.connected = False
        self._receive_callback: Optional[Callable] = None
        self._receive_thread: Optional[threading.Thread] = None
        self._running = False
    
    def connect(self) -> bool:
        """
        Open Bluetooth serial connection.
        
        Returns:
            True if successful
        """
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.BAUD_RATE,
                timeout=1
            )
            self.connected = True
            print(f"✅ Bluetooth connected on {self.port}")
            return True
        except serial.SerialException as e:
            print(f"❌ Bluetooth connection failed: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Close Bluetooth connection."""
        self._running = False
        
        if self._receive_thread:
            self._receive_thread.join(timeout=2)
        
        if self.serial and self.serial.is_open:
            self.serial.close()
        
        self.connected = False
        print("📱 Bluetooth disconnected")
    
    def is_connected(self) -> bool:
        """Check if connected."""
        return self.connected and self.serial and self.serial.is_open
    
    def send(self, data: dict) -> bool:
        """
        Send data as JSON over Bluetooth.
        
        Args:
            data: Dictionary to send
            
        Returns:
            True if successful
        """
        if not self.is_connected():
            print("❌ Cannot send - not connected")
            return False
        
        try:
            # Convert to JSON with newline terminator
            json_str = json.dumps(data) + "\n"
            self.serial.write(json_str.encode('utf-8'))
            self.serial.flush()
            print(f"📤 Sent {len(json_str)} bytes via Bluetooth")
            return True
        except Exception as e:
            print(f"❌ Send failed: {e}")
            return False
    
    def receive(self, timeout: float = 5.0) -> Optional[dict]:
        """
        Receive JSON data from Bluetooth.
        
        Args:
            timeout: Timeout in seconds
            
        Returns:
            Parsed JSON dict or None
        """
        if not self.is_connected():
            return None
        
        try:
            old_timeout = self.serial.timeout
            self.serial.timeout = timeout
            
            line = self.serial.readline().decode('utf-8').strip()
            
            self.serial.timeout = old_timeout
            
            if line:
                return json.loads(line)
            return None
        except json.JSONDecodeError:
            print("⚠️  Received invalid JSON")
            return None
        except Exception as e:
            print(f"❌ Receive error: {e}")
            return None
    
    def set_receive_callback(self, callback: Callable[[dict], None]):
        """
        Set callback for incoming messages.
        
        Args:
            callback: Function to call with received data
        """
        self._receive_callback = callback
    
    def start_receive_loop(self):
        """Start background thread to receive messages."""
        if self._receive_thread and self._receive_thread.is_alive():
            return
        
        self._running = True
        self._receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self._receive_thread.start()
        print("🔄 Bluetooth receive loop started")
    
    def _receive_loop(self):
        """Background receive loop."""
        while self._running and self.is_connected():
            try:
                data = self.receive(timeout=1.0)
                if data and self._receive_callback:
                    self._receive_callback(data)
            except:
                time.sleep(0.1)


class BluetoothManager:
    """
    High-level Bluetooth manager with automatic fallback to simulation.
    """
    
    def __init__(self, force_simulation: bool = False):
        """
        Initialize Bluetooth manager.
        
        Args:
            force_simulation: Force simulation mode even on Pi
        """
        self.simulation_mode = force_simulation or not IS_RASPBERRY_PI or not SERIAL_AVAILABLE
        
        if self.simulation_mode:
            self._driver = SimulatedBluetooth()
        else:
            self._driver = BluetoothSerial()
    
    def connect(self) -> bool:
        """Connect to Bluetooth."""
        return self._driver.connect()
    
    def disconnect(self):
        """Disconnect from Bluetooth."""
        self._driver.disconnect()
    
    def is_connected(self) -> bool:
        """Check connection status."""
        return self._driver.is_connected()
    
    def send_analysis(self, analysis_result: dict) -> bool:
        """
        Send water analysis result.
        
        Args:
            analysis_result: Result from TrustEngine.analyze_water()
            
        Returns:
            True if sent successfully
        """
        # Create compact payload for transmission
        payload = {
            "type": "ANALYSIS_RESULT",
            "ts": analysis_result.get("timestamp", ""),
            "tds": analysis_result.get("readings", {}).get("tds_ppm", 0),
            "turb": analysis_result.get("readings", {}).get("turbidity_ntu", 0),
            "temp": analysis_result.get("readings", {}).get("temperature_c", 0),
            "stability": analysis_result.get("stability", {}).get("overall", 0),
            "score": analysis_result.get("jal_score", 0),
            "verdict": analysis_result.get("verdict", "ERROR"),
            "message": analysis_result.get("verdict_message", ""),
            "profile": analysis_result.get("profile", ""),
            "alert": analysis_result.get("seasonal_alert", ""),
            "sim": analysis_result.get("simulation_mode", False)
        }
        
        return self._driver.send(payload)
    
    def send_error(self, error_message: str) -> bool:
        """
        Send error message.
        
        Args:
            error_message: Error description
            
        Returns:
            True if sent successfully
        """
        payload = {
            "type": "ERROR",
            "message": error_message,
            "ts": time.strftime("%Y-%m-%dT%H:%M:%S")
        }
        
        return self._driver.send(payload)
    
    def send_status(self, status: str) -> bool:
        """
        Send status update.
        
        Args:
            status: Status message
            
        Returns:
            True if sent successfully
        """
        payload = {
            "type": "STATUS",
            "status": status,
            "ts": time.strftime("%Y-%m-%dT%H:%M:%S")
        }
        
        return self._driver.send(payload)


# Quick test when run directly
if __name__ == "__main__":
    print("=" * 50)
    print("AQUA-MIND BLUETOOTH TEST")
    print("=" * 50)
    
    bt = BluetoothManager(force_simulation=True)
    
    # Connect
    bt.connect()
    
    # Send test analysis
    test_result = {
        "timestamp": "2026-01-15T16:00:00",
        "readings": {
            "tds_ppm": 350.5,
            "turbidity_ntu": 2.1,
            "temperature_c": 28.0
        },
        "stability": {
            "overall": 92.5
        },
        "jal_score": 75.2,
        "verdict": "CAUTION",
        "verdict_message": "Water quality marginal - treatment recommended",
        "profile": "Jabalpur, Madhya Pradesh",
        "seasonal_alert": "",
        "simulation_mode": True
    }
    
    print("\n📊 Sending test analysis...")
    bt.send_analysis(test_result)
    
    # Send status
    print("\n📊 Sending status...")
    bt.send_status("Ready for next test")
    
    # Disconnect
    bt.disconnect()
    
    print("\n✅ Bluetooth test complete!")
