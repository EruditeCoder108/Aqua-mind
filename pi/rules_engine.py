"""
Aqua-Mind Rules Engine
======================
Offline-first safety rules based on BIS IS:10500 standards.
Provides guaranteed safety advice without internet connectivity.
"""

import json
from typing import Dict, List, Optional
from dataclasses import dataclass
from enum import Enum


class Verdict(Enum):
    """Water safety verdict levels."""
    SAFE = "SAFE"
    CAUTION = "CAUTION"
    UNSAFE = "UNSAFE"
    ERROR = "ERROR"


@dataclass
class SafetyRule:
    """A single safety rule definition."""
    id: str
    name: str
    parameter: str
    condition: str  # "gt", "lt", "gte", "lte", "range"
    threshold: float
    threshold_max: Optional[float] = None  # For range conditions
    verdict: Verdict = Verdict.CAUTION
    priority: int = 5  # 1-10, higher = more important
    action: str = ""
    explanation: str = ""


class RulesEngine:
    """
    Offline-first rules engine for water safety assessment.
    """
    
    # Default rules based on BIS IS:10500:2012
    DEFAULT_RULES = [
        # Critical safety rules (highest priority)
        SafetyRule(
            id="TURB_CRITICAL",
            name="Critical Turbidity",
            parameter="turbidity_ntu",
            condition="gte",
            threshold=10.0,
            verdict=Verdict.UNSAFE,
            priority=10,
            action="DO NOT DRINK. Use multi-stage filtration + boiling for 10 minutes.",
            explanation="Very high turbidity indicates sediment, microbes, or contamination."
        ),
        SafetyRule(
            id="STABILITY_FAIL",
            name="Sensor Unstable",
            parameter="stability_score",
            condition="lt",
            threshold=50.0,
            verdict=Verdict.ERROR,
            priority=10,
            action="Clean sensor probe with soft cloth and retry. Do not trust this reading.",
            explanation="Sensor readings are inconsistent. This could indicate probe fouling."
        ),
        
        # High priority rules
        SafetyRule(
            id="TURB_HIGH",
            name="High Turbidity",
            parameter="turbidity_ntu",
            condition="gte",
            threshold=5.0,
            verdict=Verdict.UNSAFE,
            priority=8,
            action="Boil water for at least 5 minutes before consumption.",
            explanation="Turbidity above 5 NTU exceeds BIS permissible limit. Risk of pathogens."
        ),
        SafetyRule(
            id="TDS_VERY_HIGH",
            name="Very High TDS",
            parameter="tds_ppm",
            condition="gte",
            threshold=1000.0,
            verdict=Verdict.UNSAFE,
            priority=8,
            action="Use RO purifier. Not suitable for regular drinking.",
            explanation="TDS above 1000 ppm indicates heavy mineral content. May cause health issues."
        ),
        
        # Medium priority rules
        SafetyRule(
            id="TDS_HIGH",
            name="High TDS",
            parameter="tds_ppm",
            condition="gte",
            threshold=500.0,
            verdict=Verdict.CAUTION,
            priority=6,
            action="RO filter recommended for long-term use. Can be consumed occasionally.",
            explanation="TDS above 500 ppm exceeds BIS acceptable limit. Taste may be affected."
        ),
        SafetyRule(
            id="TURB_ELEVATED",
            name="Elevated Turbidity",
            parameter="turbidity_ntu",
            condition="gte",
            threshold=1.0,
            verdict=Verdict.CAUTION,
            priority=5,
            action="Use sediment filter or let water settle before use.",
            explanation="Turbidity above 1 NTU exceeds BIS acceptable limit."
        ),
        SafetyRule(
            id="STABILITY_LOW",
            name="Low Stability",
            parameter="stability_score",
            condition="lt",
            threshold=70.0,
            verdict=Verdict.CAUTION,
            priority=5,
            action="Wait 30 seconds and test again for more accurate reading.",
            explanation="Sensor readings have some variation. Results may not be fully reliable."
        ),
        
        # Temperature rules
        SafetyRule(
            id="TEMP_HIGH",
            name="High Temperature",
            parameter="temperature_c",
            condition="gte",
            threshold=35.0,
            verdict=Verdict.CAUTION,
            priority=3,
            action="Cool water before testing. Warm water may affect sensor accuracy.",
            explanation="Water temperature is high. TDS readings may be elevated."
        ),
        SafetyRule(
            id="TEMP_LOW",
            name="Low Temperature",
            parameter="temperature_c",
            condition="lt",
            threshold=10.0,
            verdict=Verdict.CAUTION,
            priority=3,
            action="Allow water to reach room temperature for accurate testing.",
            explanation="Cold water may affect sensor calibration."
        ),
    ]
    
    def __init__(self, custom_rules: Optional[List[SafetyRule]] = None):
        """
        Initialize rules engine.
        
        Args:
            custom_rules: Optional list of custom rules to add
        """
        self.rules = self.DEFAULT_RULES.copy()
        
        if custom_rules:
            self.rules.extend(custom_rules)
        
        # Sort by priority (highest first)
        self.rules.sort(key=lambda r: r.priority, reverse=True)
    
    def _evaluate_condition(self, value: float, rule: SafetyRule) -> bool:
        """Check if a value matches a rule's condition."""
        if rule.condition == "gt":
            return value > rule.threshold
        elif rule.condition == "gte":
            return value >= rule.threshold
        elif rule.condition == "lt":
            return value < rule.threshold
        elif rule.condition == "lte":
            return value <= rule.threshold
        elif rule.condition == "range":
            return rule.threshold <= value <= (rule.threshold_max or rule.threshold)
        return False
    
    def evaluate(self, readings: Dict) -> Dict:
        """
        Evaluate all rules against sensor readings.
        
        Args:
            readings: Dict with keys like 'tds_ppm', 'turbidity_ntu', etc.
            
        Returns:
            Dict with verdict, triggered rules, and actions
        """
        triggered_rules = []
        
        # Map reading keys to rule parameters
        param_map = {
            "tds_ppm": readings.get("tds_ppm", 0),
            "turbidity_ntu": readings.get("turbidity_ntu", 0),
            "temperature_c": readings.get("temperature_c", 25),
            "stability_score": readings.get("stability_score", 100),
        }
        
        # Evaluate each rule
        for rule in self.rules:
            if rule.parameter in param_map:
                value = param_map[rule.parameter]
                if self._evaluate_condition(value, rule):
                    triggered_rules.append({
                        "id": rule.id,
                        "name": rule.name,
                        "verdict": rule.verdict.value,
                        "priority": rule.priority,
                        "action": rule.action,
                        "explanation": rule.explanation,
                        "parameter": rule.parameter,
                        "value": value,
                        "threshold": rule.threshold
                    })
        
        # Determine overall verdict (highest priority triggered rule wins)
        if not triggered_rules:
            overall_verdict = Verdict.SAFE.value
            primary_action = "Water appears safe for consumption."
            primary_explanation = "All parameters within acceptable limits."
        else:
            # Get the highest priority triggered rule
            primary_rule = triggered_rules[0]
            overall_verdict = primary_rule["verdict"]
            primary_action = primary_rule["action"]
            primary_explanation = primary_rule["explanation"]
        
        # Compile all unique actions
        all_actions = []
        seen_actions = set()
        for rule in triggered_rules:
            if rule["action"] not in seen_actions:
                all_actions.append(rule["action"])
                seen_actions.add(rule["action"])
        
        return {
            "verdict": overall_verdict,
            "primary_action": primary_action,
            "primary_explanation": primary_explanation,
            "all_actions": all_actions,
            "triggered_rules": triggered_rules,
            "rules_checked": len(self.rules),
            "readings_used": param_map
        }
    
    def get_safety_advice(self, verdict: str) -> Dict:
        """
        Get general safety advice for a verdict level.
        
        Args:
            verdict: One of SAFE, CAUTION, UNSAFE, ERROR
            
        Returns:
            Dict with advice and recommendations
        """
        advice = {
            "SAFE": {
                "color": "green",
                "icon": "✅",
                "short": "पीने योग्य (Drinkable)",
                "long": "Water quality is within safe limits. You can consume this water.",
                "precautions": [
                    "Store in clean containers",
                    "Keep away from direct sunlight"
                ]
            },
            "CAUTION": {
                "color": "yellow",
                "icon": "⚠️",
                "short": "सावधानी (Caution)",
                "long": "Water quality is marginal. Treatment recommended before drinking.",
                "precautions": [
                    "Use water filter if available",
                    "Boiling is recommended",
                    "Not ideal for infants or elderly"
                ]
            },
            "UNSAFE": {
                "color": "red",
                "icon": "🚫",
                "short": "असुरक्षित (Unsafe)",
                "long": "पानी पीने योग्य नहीं है। उबाल कर पीएं।",
                "precautions": [
                    "DO NOT drink without treatment",
                    "Boil for at least 10 minutes",
                    "Use RO/UV purifier if available",
                    "Consider alternative water source"
                ]
            },
            "ERROR": {
                "color": "gray",
                "icon": "❌",
                "short": "त्रुटि (Error)",
                "long": "Sensor error detected. Reading is not reliable.",
                "precautions": [
                    "Clean sensor probe",
                    "Check connections",
                    "Wait 30 seconds and retry",
                    "Do not trust this reading"
                ]
            }
        }
        
        return advice.get(verdict, advice["ERROR"])
    
    def format_for_display(self, evaluation: Dict) -> str:
        """
        Format evaluation result for console/OLED display.
        
        Args:
            evaluation: Result from evaluate()
            
        Returns:
            Formatted string for display
        """
        advice = self.get_safety_advice(evaluation["verdict"])
        
        lines = [
            f"{advice['icon']} {advice['short']}",
            "",
            f"TDS: {evaluation['readings_used']['tds_ppm']:.0f} ppm",
            f"Turb: {evaluation['readings_used']['turbidity_ntu']:.1f} NTU",
            f"Temp: {evaluation['readings_used']['temperature_c']:.1f}°C",
            "",
            evaluation["primary_action"][:50]  # Truncate for display
        ]
        
        return "\n".join(lines)
    
    def format_for_json(self, evaluation: Dict) -> Dict:
        """
        Format evaluation for JSON transmission (Bluetooth).
        
        Args:
            evaluation: Result from evaluate()
            
        Returns:
            JSON-ready dict
        """
        advice = self.get_safety_advice(evaluation["verdict"])
        
        return {
            "verdict": evaluation["verdict"],
            "icon": advice["icon"],
            "short_hi": advice["short"],
            "long_en": advice["long"],
            "color": advice["color"],
            "action": evaluation["primary_action"],
            "explanation": evaluation["primary_explanation"],
            "precautions": advice["precautions"],
            "readings": evaluation["readings_used"],
            "triggered_count": len(evaluation["triggered_rules"])
        }


# Quick test when run directly
if __name__ == "__main__":
    print("=" * 50)
    print("AQUA-MIND RULES ENGINE TEST")
    print("=" * 50)
    
    engine = RulesEngine()
    
    # Test scenarios
    scenarios = [
        {"name": "Clean Water", "tds_ppm": 150, "turbidity_ntu": 0.5, "temperature_c": 25, "stability_score": 95},
        {"name": "Tap Water", "tds_ppm": 400, "turbidity_ntu": 2.0, "temperature_c": 28, "stability_score": 85},
        {"name": "Dirty Water", "tds_ppm": 650, "turbidity_ntu": 8.0, "temperature_c": 30, "stability_score": 75},
        {"name": "Contaminated", "tds_ppm": 1200, "turbidity_ntu": 15.0, "temperature_c": 32, "stability_score": 60},
        {"name": "Sensor Error", "tds_ppm": 300, "turbidity_ntu": 3.0, "temperature_c": 25, "stability_score": 30},
    ]
    
    for scenario in scenarios:
        name = scenario.pop("name")
        print(f"\n📋 Scenario: {name}")
        print("-" * 30)
        
        result = engine.evaluate(scenario)
        print(engine.format_for_display(result))
        
        scenario["name"] = name  # Restore for next iteration
    
    print("\n" + "=" * 50)
    print("✅ Rules Engine Test Complete!")
