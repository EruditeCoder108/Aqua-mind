"""
Aqua-Mind Main Orchestrator
===========================
The main entry point that ties everything together.
Handles the main loop, button input, and coordinates all modules.

Usage:
    python main.py                    # Normal mode (simulation on PC)
    python main.py --scenario dirty   # Test with dirty water scenario
    python main.py --profile JAIPUR   # Use Jaipur profile
"""

import argparse
import json
import time
import sys
from datetime import datetime

# Import our modules
from sensors import SensorManager
from trust_engine import TrustEngine
from rules_engine import RulesEngine
from bluetooth_comm import BluetoothManager


class AquaMind:
    """
    Main Aqua-Mind orchestrator class.
    """
    
    VERSION = "1.0.0"
    
    def __init__(self, profile: str = "JABALPUR", simulation_scenario: str = None):
        """
        Initialize Aqua-Mind system.
        
        Args:
            profile: Regional profile name (e.g., "JABALPUR", "JAIPUR")
            simulation_scenario: Optional simulation scenario for testing
        """
        print("\n" + "=" * 60)
        print(f"  🌊 AQUA-MIND Water Quality Intelligence v{self.VERSION}")
        print(f"  📍 Profile: {profile}")
        print("=" * 60)
        
        # Initialize components
        print("\n📦 Initializing components...")
        
        self.sensors = SensorManager(simulation_scenario=simulation_scenario)
        self.trust_engine = TrustEngine(self.sensors, profile_name=profile)
        self.rules_engine = RulesEngine()
        self.bluetooth = BluetoothManager()
        
        # State
        self.running = False
        self.last_analysis = None
        self.analysis_count = 0
        
        print("\n✅ Aqua-Mind ready!")
        print("-" * 60)
    
    def set_profile(self, profile_name: str) -> bool:
        """Change the regional profile."""
        return self.trust_engine.set_profile(profile_name)
    
    def set_scenario(self, scenario: str):
        """Change simulation scenario (for testing)."""
        self.sensors.set_scenario(scenario)
    
    def analyze_once(self) -> dict:
        """
        Perform a single water analysis.
        
        Returns:
            Complete analysis result
        """
        print("\n" + "=" * 60)
        print(f"  🔬 WATER ANALYSIS #{self.analysis_count + 1}")
        print(f"  ⏰ {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print("=" * 60)
        
        # Run trust engine analysis
        analysis = self.trust_engine.analyze_water()
        
        # Run rules engine for additional safety checks
        rules_input = {
            "tds_ppm": analysis["readings"]["tds_ppm"],
            "turbidity_ntu": analysis["readings"]["turbidity_ntu"],
            "temperature_c": analysis["readings"]["temperature_c"],
            "stability_score": analysis["stability"]["overall"]
        }
        
        rules_result = self.rules_engine.evaluate(rules_input)
        
        # Merge results
        analysis["rules"] = {
            "triggered": len(rules_result["triggered_rules"]),
            "actions": rules_result["all_actions"],
            "primary_action": rules_result["primary_action"]
        }
        
        # Get display-friendly advice
        advice = self.rules_engine.get_safety_advice(analysis["verdict"])
        analysis["advice"] = advice
        
        # Store and count
        self.last_analysis = analysis
        self.analysis_count += 1
        
        # Print summary
        self._print_summary(analysis)
        
        return analysis
    
    def _print_summary(self, analysis: dict):
        """Print analysis summary to console."""
        verdict = analysis["verdict"]
        advice = analysis["advice"]
        
        print("\n" + "-" * 60)
        print(f"  {advice['icon']} RESULT: {advice['short']}")
        print("-" * 60)
        
        print(f"\n  📊 READINGS:")
        print(f"     TDS:        {analysis['readings']['tds_ppm']:.1f} ppm")
        print(f"     Turbidity:  {analysis['readings']['turbidity_ntu']:.2f} NTU")
        print(f"     Temperature:{analysis['readings']['temperature_c']:.1f}°C")
        print(f"     Stability:  {analysis['stability']['overall']:.1f}%")
        
        print(f"\n  🎯 JAL-SCORE:  {analysis['jal_score']}")
        
        print(f"\n  💬 ADVICE:")
        print(f"     {advice['long']}")
        
        if analysis["rules"]["actions"]:
            print(f"\n  ⚡ ACTIONS:")
            for i, action in enumerate(analysis["rules"]["actions"][:3], 1):
                print(f"     {i}. {action}")
        
        if analysis.get("seasonal_alert"):
            print(f"\n  ⚠️  SEASONAL ALERT:")
            print(f"     {analysis['seasonal_alert']}")
        
        print("\n" + "-" * 60)
    
    def send_to_app(self, analysis: dict = None) -> bool:
        """
        Send analysis result to mobile app via Bluetooth.
        
        Args:
            analysis: Analysis to send (uses last if None)
            
        Returns:
            True if sent successfully
        """
        if analysis is None:
            analysis = self.last_analysis
        
        if analysis is None:
            print("❌ No analysis to send!")
            return False
        
        if not self.bluetooth.is_connected():
            print("📱 Connecting to Bluetooth...")
            if not self.bluetooth.connect():
                print("❌ Bluetooth connection failed")
                return False
        
        return self.bluetooth.send_analysis(analysis)
    
    def run_interactive(self):
        """
        Run interactive mode with menu.
        """
        self.running = True
        
        print("\n📋 INTERACTIVE MODE")
        print("=" * 40)
        print("Commands:")
        print("  [1] Analyze water")
        print("  [2] Send to app")
        print("  [3] Change profile")
        print("  [4] Change scenario (sim only)")
        print("  [5] View last result")
        print("  [q] Quit")
        print("=" * 40)
        
        while self.running:
            try:
                cmd = input("\n> Enter command: ").strip().lower()
                
                if cmd == "1":
                    self.analyze_once()
                    
                elif cmd == "2":
                    self.send_to_app()
                    
                elif cmd == "3":
                    profiles = self.trust_engine.geo_profile.list_profiles()
                    print(f"Available profiles: {', '.join(profiles)}")
                    profile = input("Enter profile name: ").strip().upper()
                    self.set_profile(profile)
                    
                elif cmd == "4":
                    scenarios = ["clean_water", "tap_water", "dirty_water", "contaminated", "sensor_error"]
                    print(f"Available scenarios: {', '.join(scenarios)}")
                    scenario = input("Enter scenario: ").strip()
                    self.set_scenario(scenario)
                    
                elif cmd == "5":
                    if self.last_analysis:
                        print(json.dumps(self.last_analysis, indent=2))
                    else:
                        print("No analysis yet. Run analysis first.")
                        
                elif cmd == "q" or cmd == "quit" or cmd == "exit":
                    self.running = False
                    print("👋 Goodbye!")
                    
                else:
                    print("Unknown command. Try 1, 2, 3, 4, 5, or q")
                    
            except KeyboardInterrupt:
                print("\n\n👋 Interrupted. Goodbye!")
                self.running = False
            except Exception as e:
                print(f"❌ Error: {e}")
    
    def run_continuous(self, interval: int = 60):
        """
        Run continuous monitoring mode.
        
        Args:
            interval: Seconds between analyses
        """
        print(f"\n🔄 CONTINUOUS MODE (interval: {interval}s)")
        print("   Press Ctrl+C to stop")
        print("-" * 40)
        
        self.running = True
        
        try:
            while self.running:
                # Analyze
                analysis = self.analyze_once()
                
                # Send to app
                self.send_to_app(analysis)
                
                # Wait
                print(f"\n⏳ Next analysis in {interval} seconds...")
                time.sleep(interval)
                
        except KeyboardInterrupt:
            print("\n\n👋 Stopped. Goodbye!")
            self.running = False
    
    def cleanup(self):
        """Clean up resources."""
        self.bluetooth.disconnect()
        self.sensors.cleanup()
        print("🧹 Cleanup complete")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Aqua-Mind Water Quality Intelligence System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python main.py                           # Interactive mode
  python main.py --scenario dirty          # Test with dirty water
  python main.py --profile JAIPUR          # Use Jaipur profile
  python main.py --continuous --interval 30 # Monitor every 30 seconds

Available profiles: JABALPUR, JAIPUR, CHENNAI, DELHI, GUWAHATI, MUMBAI
Available scenarios: clean_water, tap_water, dirty_water, contaminated, sensor_error
        """
    )
    
    parser.add_argument(
        "--profile", "-p",
        default="JABALPUR",
        help="Regional profile to use (default: JABALPUR)"
    )
    
    parser.add_argument(
        "--scenario", "-s",
        default=None,
        help="Simulation scenario (for testing without hardware)"
    )
    
    parser.add_argument(
        "--continuous", "-c",
        action="store_true",
        help="Run in continuous monitoring mode"
    )
    
    parser.add_argument(
        "--interval", "-i",
        type=int,
        default=60,
        help="Interval between analyses in continuous mode (seconds)"
    )
    
    parser.add_argument(
        "--single",
        action="store_true",
        help="Run single analysis and exit"
    )
    
    args = parser.parse_args()
    
    # Create Aqua-Mind instance
    aqua = AquaMind(
        profile=args.profile,
        simulation_scenario=args.scenario
    )
    
    try:
        if args.single:
            # Single analysis mode
            analysis = aqua.analyze_once()
            aqua.send_to_app(analysis)
            
        elif args.continuous:
            # Continuous monitoring mode
            aqua.run_continuous(interval=args.interval)
            
        else:
            # Interactive mode
            aqua.run_interactive()
            
    finally:
        aqua.cleanup()


if __name__ == "__main__":
    main()
