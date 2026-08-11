import time
from typing import Dict, Tuple

# ------------------ CONFIGURABLE THRESHOLDS ------------------
THRESHOLDS = {
    'TDS_WATER_MAX': 650.0,
    'TDS_SALT_MIN': 2000.0,
    'TDS_SUGAR_MIN': 1200.0,
    'TDS_SUGAR_MAX': 1800.0,
    'NH3_SPOILED_MIN': 20.0,
    'PH_UREA_MIN': 7.0,
    'NH3_SLOPE_UREA_MIN': 0.15,
    'STARCH_STABLE_SEC': 10.0
}

# ------------------ CORE CLASSIFIER --------------------------
def classify(pH: float, TDS: float, NH3: float, 
             stable_duration: float, nh3_slope: float) -> Dict[str, object]:
    """
    Classifies milk sample into one of 7 categories.
    
    Args:
        pH: measured pH value
        TDS: total dissolved solids in ppm
        NH3: ammonia concentration in ppm
        stable_duration: seconds TDS remained stable
        nh3_slope: rate of NH3 change (ppm/sec)
    
    Returns:
        Dict with keys: status, reason, confidence
    """
    # Priority 1: Spoilage (health hazard)
    if NH3 >= THRESHOLDS['NH3_SPOILED_MIN']:
        return {
            'status': 'SPOILED',
            'reason': f'NH3={NH3:.2f} ≥ {THRESHOLDS["NH3_SPOILED_MIN"]}',
            'confidence': 0.95
        }
    
    # Priority 2: Starch (physical property)
    if stable_duration >= THRESHOLDS['STARCH_STABLE_SEC']:
        return {
            'status': 'STARCH',
            'reason': f'Stable for {stable_duration:.1f}s',
            'confidence': 0.90
        }
    
    # Priority 3: Urea (requires both pH and slope)
    if pH > THRESHOLDS['PH_UREA_MIN'] and nh3_slope > THRESHOLDS['NH3_SLOPE_UREA_MIN']:
        return {
            'status': 'UREA',
            'reason': f'pH={pH:.2f}, slope={nh3_slope:.3f}',
            'confidence': 0.88
        }
    
    # Priority 4: Salt
    if TDS > THRESHOLDS['TDS_SALT_MIN']:
        return {
            'status': 'SALT',
            'reason': f'TDS={TDS:.1f} > {THRESHOLDS["TDS_SALT_MIN"]}',
            'confidence': 0.92
        }
    
    # Priority 5: Sugar
    if THRESHOLDS['TDS_SUGAR_MIN'] <= TDS <= THRESHOLDS['TDS_SUGAR_MAX']:
        return {
            'status': 'SUGAR',
            'reason': f'TDS={TDS:.1f} in [{THRESHOLDS["TDS_SUGAR_MIN"]}, {THRESHOLDS["TDS_SUGAR_MAX"]}]',
            'confidence': 0.85
        }
    
    # Priority 6: Water
    if TDS < THRESHOLDS['TDS_WATER_MAX']:
        return {
            'status': 'WATER',
            'reason': f'TDS={TDS:.1f} < {THRESHOLDS["TDS_WATER_MAX"]}',
            'confidence': 0.90
        }
    
    # Default: Pure
    return {
        'status': 'PURE',
        'reason': 'All parameters within normal range',
        'confidence': 0.80
    }

# ------------------ HELPER FOR BATCH TESTING -----------------
def batch_classify(test_data: list) -> list:
    """Runs classify() on a list of test tuples."""
    results = []
    for pH, TDS, NH3, dur, slope in test_data:
        results.append(classify(pH, TDS, NH3, dur, slope))
    return results