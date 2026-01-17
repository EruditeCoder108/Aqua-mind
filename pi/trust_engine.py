"""
Aqua-Mind Trust Engine
======================
The 5-Pillar Trust System for validating sensor data.

Pillars:
1. Statistical Tri-Check - Burst sampling for noise reduction
2. Stability Index - Variance analysis over time
3. Geo-Adaptive Profiling - Region-specific thresholds
4. Rule Engine Integration - Safety rule application
5. AI Enhancement - Gemini integration (handled in mobile app)
"""

import time
import json
import math
from pathlib import Path
from datetime import datetime
from typing import Callable, Dict, Tuple, List, Optional


class TriCheck:
    """
    Pillar 1: Statistical Tri-Check
    Takes burst samples to detect noise and calculate stability.
    """
    
    def __init__(self, bursts=3, samples_per_burst=5, burst_delay=0.2, sample_delay=0.01):
        """
        Configure Tri-Check parameters.
        
        Args:
            bursts: Number of burst readings (default 3)
            samples_per_burst: Samples within each burst (default 5)
            burst_delay: Delay between bursts in seconds (default 0.2)
            sample_delay: Delay between samples in seconds (default 0.01)
        """
        self.bursts = bursts
        self.samples_per_burst = samples_per_burst
        self.burst_delay = burst_delay
        self.sample_delay = sample_delay
    
    def read_with_validation(self, sensor_func: Callable[[], float]) -> Tuple[float, float, List[float]]:
        """
        Take burst samples and calculate stability.
        
        Args:
            sensor_func: A function that returns a sensor reading
            
        Returns:
            Tuple of (mean_value, stability_score_0_to_100, burst_means)
        """
        burst_means = []
        
        for i in range(self.bursts):
            samples = []
            
            for _ in range(self.samples_per_burst):
                samples.append(sensor_func())
                time.sleep(self.sample_delay)
            
            burst_mean = sum(samples) / len(samples)
            burst_means.append(burst_mean)
            
            if i < self.bursts - 1:
                time.sleep(self.burst_delay)
        
        # Calculate overall statistics
        overall_mean = sum(burst_means) / len(burst_means)
        
        if len(burst_means) < 2 or overall_mean == 0:
            return overall_mean, 100.0, burst_means
        
        # Calculate standard deviation
        variance = sum((x - overall_mean) ** 2 for x in burst_means) / len(burst_means)
        std_dev = math.sqrt(variance)
        
        # Calculate stability score
        # If std_dev is 0, score is 100
        # If std_dev is 10% of mean, score is ~50
        variation_percent = (std_dev / overall_mean) * 100
        stability_score = max(0, 100 - (variation_percent * 5))  # 5x penalty factor
        
        return overall_mean, round(stability_score, 1), burst_means


class StabilityTracker:
    """
    Pillar 2: Stability Index
    Tracks reading stability over time to detect sensor drift.
    """
    
    def __init__(self, window_size=10):
        """
        Args:
            window_size: Number of recent readings to track
        """
        self.window_size = window_size
        self.history: Dict[str, List[float]] = {
            "tds": [],
            "turbidity": [],
            "temperature": []
        }
    
    def add_reading(self, sensor: str, value: float):
        """Add a reading to the history."""
        if sensor in self.history:
            self.history[sensor].append(value)
            if len(self.history[sensor]) > self.window_size:
                self.history[sensor].pop(0)
    
    def get_trend(self, sensor: str) -> Dict:
        """
        Analyze trend for a sensor.
        
        Returns:
            Dict with trend analysis: direction, magnitude, stability
        """
        if sensor not in self.history or len(self.history[sensor]) < 3:
            return {"direction": "unknown", "magnitude": 0, "stable": True}
        
        values = self.history[sensor]
        
        # Calculate linear trend
        n = len(values)
        x_mean = (n - 1) / 2
        y_mean = sum(values) / n
        
        numerator = sum((i - x_mean) * (v - y_mean) for i, v in enumerate(values))
        denominator = sum((i - x_mean) ** 2 for i in range(n))
        
        slope = numerator / denominator if denominator != 0 else 0
        
        # Determine direction
        if abs(slope) < 0.5:
            direction = "stable"
        elif slope > 0:
            direction = "rising"
        else:
            direction = "falling"
        
        # Calculate stability (coefficient of variation)
        if y_mean != 0:
            variance = sum((v - y_mean) ** 2 for v in values) / n
            cv = (math.sqrt(variance) / abs(y_mean)) * 100
            stable = cv < 15  # Less than 15% variation is stable
        else:
            cv = 0
            stable = True
        
        return {
            "direction": direction,
            "magnitude": round(abs(slope), 2),
            "cv_percent": round(cv, 1),
            "stable": stable,
            "samples": len(values)
        }
    
    def clear(self):
        """Clear all history."""
        for key in self.history:
            self.history[key] = []


class GeoProfile:
    """
    Pillar 3: Geo-Adaptive Profiling
    Adjusts thresholds and weights based on regional characteristics.
    """
    
    def __init__(self, profiles_path: Optional[str] = None):
        """
        Load profiles from JSON file.
        
        Args:
            profiles_path: Path to profiles.json (default: same directory)
        """
        if profiles_path is None:
            profiles_path = Path(__file__).parent / "profiles.json"
        
        self.profiles_path = Path(profiles_path)
        self._load_profiles()
        
        # Set default profile
        self.current_profile_name = self.profiles_data.get("default_profile", "JABALPUR")
        self.current_profile = self.profiles.get(self.current_profile_name, {})
    
    def _load_profiles(self):
        """Load profiles from file."""
        try:
            with open(self.profiles_path, 'r') as f:
                self.profiles_data = json.load(f)
                self.profiles = self.profiles_data.get("profiles", {})
                self.bis_standards = self.profiles_data.get("bis_standards", {})
        except FileNotFoundError:
            print(f"⚠️  Profiles file not found: {self.profiles_path}")
            self.profiles_data = {}
            self.profiles = {}
            self.bis_standards = {}
        except json.JSONDecodeError as e:
            print(f"⚠️  Error parsing profiles: {e}")
            self.profiles_data = {}
            self.profiles = {}
            self.bis_standards = {}
    
    def set_profile(self, profile_name: str) -> bool:
        """
        Switch to a different regional profile.
        
        Args:
            profile_name: Name of the profile (e.g., "JABALPUR", "JAIPUR")
            
        Returns:
            True if successful, False if profile not found
        """
        profile_name = profile_name.upper()
        
        if profile_name in self.profiles:
            self.current_profile_name = profile_name
            self.current_profile = self.profiles[profile_name]
            print(f"📍 Profile set to: {self.current_profile.get('name', profile_name)}")
            return True
        else:
            print(f"⚠️  Profile '{profile_name}' not found. Available: {list(self.profiles.keys())}")
            return False
    
    def get_weights(self) -> Dict[str, float]:
        """Get current sensor weights."""
        return {
            "tds": self.current_profile.get("tds_weight", 0.5),
            "turbidity": self.current_profile.get("turb_weight", 0.4),
            "temperature": self.current_profile.get("temp_weight", 0.1)
        }
    
    def get_thresholds(self) -> Dict:
        """Get current thresholds."""
        return self.current_profile.get("thresholds", {
            "tds_safe": 300,
            "tds_caution": 500,
            "tds_unsafe": 900,
            "turb_safe": 1,
            "turb_caution": 5,
            "turb_unsafe": 10
        })
    
    def get_seasonal_modifier(self) -> Dict:
        """
        Get seasonal adjustment for current month.
        
        Returns:
            Dict with modifiers and alert message if applicable
        """
        current_month = datetime.now().month
        seasonal = self.current_profile.get("seasonal_adjustments", {})
        
        for season, config in seasonal.items():
            if current_month in config.get("months", []):
                return {
                    "season": season,
                    "tds_modifier": config.get("tds_weight_modifier", 1.0),
                    "turb_modifier": config.get("turb_weight_modifier", 1.0),
                    "alert": config.get("alert", "")
                }
        
        return {
            "season": "normal",
            "tds_modifier": 1.0,
            "turb_modifier": 1.0,
            "alert": ""
        }
    
    def is_strict_mode(self) -> bool:
        """Check if current profile uses strict mode."""
        return self.current_profile.get("strict_mode", True)
    
    def get_profile_info(self) -> Dict:
        """Get current profile information."""
        return {
            "name": self.current_profile_name,
            "full_name": self.current_profile.get("name", self.current_profile_name),
            "zone": self.current_profile.get("zone", "Unknown"),
            "description": self.current_profile.get("description", ""),
            "common_contaminants": self.current_profile.get("common_contaminants", [])
        }
    
    def list_profiles(self) -> List[str]:
        """List all available profile names."""
        return list(self.profiles.keys())


class JalScoreCalculator:
    """
    Combines all pillars to calculate the final Jal-Score.
    """
    
    def __init__(self, geo_profile: GeoProfile):
        """
        Args:
            geo_profile: GeoProfile instance for weights and thresholds
        """
        self.geo = geo_profile
    
    def calculate(self, tds_ppm: float, turbidity_ntu: float, 
                  stability_score: float, temperature: float = 25.0) -> Dict:
        """
        Calculate the Jal-Score based on all sensor data.
        
        Args:
            tds_ppm: TDS reading in ppm
            turbidity_ntu: Turbidity reading in NTU
            stability_score: Sensor stability (0-100)
            temperature: Temperature in Celsius
            
        Returns:
            Dict with score, verdict, and detailed breakdown
        """
        thresholds = self.geo.get_thresholds()
        weights = self.geo.get_weights()
        seasonal = self.geo.get_seasonal_modifier()
        
        # Apply seasonal modifiers to weights
        w_tds = weights["tds"] * seasonal.get("tds_modifier", 1.0)
        w_turb = weights["turbidity"] * seasonal.get("turb_modifier", 1.0)
        
        # Normalize weights
        total_weight = w_tds + w_turb
        w_tds /= total_weight
        w_turb /= total_weight
        
        # Calculate risk scores (0-100, higher = worse)
        tds_risk = min(100, (tds_ppm / thresholds.get("tds_unsafe", 900)) * 100)
        turb_risk = min(100, (turbidity_ntu / thresholds.get("turb_unsafe", 10)) * 100)
        
        # Stability penalty (if sensor is unstable, reduce confidence)
        stability_penalty = (100 - stability_score) * 0.5
        
        # Calculate final Jal-Score (0-100, higher = better)
        jal_score = 100 - (tds_risk * w_tds) - (turb_risk * w_turb) - stability_penalty
        jal_score = max(0, min(100, round(jal_score, 1)))
        
        # Determine verdict
        if stability_score < 50:
            verdict = "ERROR"
            verdict_message = "Sensor unstable - clean probe and retry"
        elif jal_score >= 80:
            verdict = "SAFE"
            verdict_message = "Water appears safe for consumption"
        elif jal_score >= 50:
            verdict = "CAUTION"
            verdict_message = "Water quality marginal - treatment recommended"
        else:
            verdict = "UNSAFE"
            verdict_message = "Water unsafe - do not consume without treatment"
        
        # Strict mode adjustments
        if self.geo.is_strict_mode() and verdict == "CAUTION":
            # In strict mode, CAUTION becomes more serious
            verdict_message += " (Strict Mode: Consider treatment)"
        
        return {
            "jal_score": jal_score,
            "verdict": verdict,
            "verdict_message": verdict_message,
            "breakdown": {
                "tds_risk": round(tds_risk, 1),
                "turb_risk": round(turb_risk, 1),
                "stability_penalty": round(stability_penalty, 1),
                "weights_used": {"tds": round(w_tds, 2), "turb": round(w_turb, 2)}
            },
            "seasonal_alert": seasonal.get("alert", ""),
            "profile": self.geo.get_profile_info()["full_name"],
            "strict_mode": self.geo.is_strict_mode()
        }


class TrustEngine:
    """
    Main Trust Engine - combines all pillars for complete analysis.
    """
    
    def __init__(self, sensor_manager, profile_name: str = None):
        """
        Initialize the Trust Engine.
        
        Args:
            sensor_manager: SensorManager instance
            profile_name: Optional profile name to use
        """
        self.sensors = sensor_manager
        self.tri_check = TriCheck()
        self.stability_tracker = StabilityTracker()
        self.geo_profile = GeoProfile()
        self.jal_calculator = JalScoreCalculator(self.geo_profile)
        
        if profile_name:
            self.geo_profile.set_profile(profile_name)
    
    def set_profile(self, profile_name: str) -> bool:
        """Change the regional profile."""
        return self.geo_profile.set_profile(profile_name)
    
    def analyze_water(self) -> Dict:
        """
        Perform complete water quality analysis.
        
        Returns:
            Complete analysis result with all data and recommendations
        """
        print("\n🔬 Starting water analysis...")
        print("=" * 40)
        
        # Pillar 1: Tri-Check for TDS
        print("📊 Running TDS Tri-Check...")
        tds_mean, tds_stability, tds_bursts = self.tri_check.read_with_validation(
            self.sensors.read_tds_ppm
        )
        
        # Pillar 1: Tri-Check for Turbidity
        print("📊 Running Turbidity Tri-Check...")
        turb_mean, turb_stability, turb_bursts = self.tri_check.read_with_validation(
            self.sensors.read_turbidity_ntu
        )
        
        # Get temperature (single read is fine for temp)
        temperature = self.sensors.read_temperature()
        
        # Combined stability score
        overall_stability = (tds_stability + turb_stability) / 2
        
        # Pillar 2: Update stability tracker
        self.stability_tracker.add_reading("tds", tds_mean)
        self.stability_tracker.add_reading("turbidity", turb_mean)
        self.stability_tracker.add_reading("temperature", temperature)
        
        tds_trend = self.stability_tracker.get_trend("tds")
        turb_trend = self.stability_tracker.get_trend("turbidity")
        
        # Pillar 3 & 4: Calculate Jal-Score with geo-profile
        jal_result = self.jal_calculator.calculate(
            tds_mean, turb_mean, overall_stability, temperature
        )
        
        # Compile complete result
        result = {
            "timestamp": datetime.now().isoformat(),
            "readings": {
                "tds_ppm": round(tds_mean, 1),
                "turbidity_ntu": round(turb_mean, 2),
                "temperature_c": temperature,
            },
            "stability": {
                "tds_stability": tds_stability,
                "turb_stability": turb_stability,
                "overall": round(overall_stability, 1)
            },
            "trends": {
                "tds": tds_trend,
                "turbidity": turb_trend
            },
            "jal_score": jal_result["jal_score"],
            "verdict": jal_result["verdict"],
            "verdict_message": jal_result["verdict_message"],
            "breakdown": jal_result["breakdown"],
            "profile": jal_result["profile"],
            "seasonal_alert": jal_result["seasonal_alert"],
            "strict_mode": jal_result["strict_mode"],
            "simulation_mode": self.sensors.simulation_mode,
            "raw_data": {
                "tds_bursts": [round(b, 1) for b in tds_bursts],
                "turb_bursts": [round(b, 2) for b in turb_bursts]
            }
        }
        
        # Print summary
        print(f"\n✅ Analysis Complete!")
        print(f"   TDS: {result['readings']['tds_ppm']} ppm (stability: {tds_stability}%)")
        print(f"   Turbidity: {result['readings']['turbidity_ntu']} NTU (stability: {turb_stability}%)")
        print(f"   Temperature: {temperature}°C")
        print(f"\n   🎯 JAL-SCORE: {result['jal_score']}")
        print(f"   📋 VERDICT: {result['verdict']}")
        print(f"   💬 {result['verdict_message']}")
        
        if result['seasonal_alert']:
            print(f"\n   ⚠️  SEASONAL ALERT: {result['seasonal_alert']}")
        
        return result


# Quick test when run directly
if __name__ == "__main__":
    print("=" * 50)
    print("AQUA-MIND TRUST ENGINE TEST")
    print("=" * 50)
    
    # Import sensor manager
    from sensors import SensorManager
    
    # Create sensor manager in simulation mode
    sensors = SensorManager(simulation_scenario="tap_water")
    
    # Create trust engine
    engine = TrustEngine(sensors, profile_name="JABALPUR")
    
    # Run analysis
    result = engine.analyze_water()
    
    print("\n" + "=" * 50)
    print("Full JSON Result:")
    print(json.dumps(result, indent=2))
