"""
classifier.py
M1 - Day 2: Decision-tree classification logic.
"""

from thresholds_config import THRESHOLDS

def classify(pH: float, TDS: float, NH3: float,
             stable_duration: float, nh3_slope: float) -> dict:
    """
    Rule-based decision tree for milk adulteration detection.
    Returns: { 'status': str, 'reason': str, 'confidence': float }
    """
    # 1. Spoilage (highest priority)
    if NH3 >= THRESHOLDS['NH3_SPOILED_MIN']:
        return {
            'status': 'SPOILED',
            'reason': f'NH3={NH3:.2f} >= {THRESHOLDS["NH3_SPOILED_MIN"]}',
            'confidence': 0.95
        }

    # 2. Starch
    if stable_duration >= THRESHOLDS['STARCH_STABLE_SEC']:
        return {
            'status': 'STARCH',
            'reason': f'stable for {stable_duration:.1f}s',
            'confidence': 0.90
        }

    # 3. Urea
    if pH > THRESHOLDS['PH_UREA_MIN'] and nh3_slope > THRESHOLDS['NH3_SLOPE_UREA_MIN']:
        return {
            'status': 'UREA',
            'reason': f'pH={pH:.2f}, slope={nh3_slope:.3f}',
            'confidence': 0.88
        }

    # 4. Salt
    if TDS > THRESHOLDS['TDS_SALT_MIN']:
        return {
            'status': 'SALT',
            'reason': f'TDS={TDS:.1f} > {THRESHOLDS["TDS_SALT_MIN"]}',
            'confidence': 0.92
        }

    # 5. Sugar (with hysteresis fix from Day 6)
    if THRESHOLDS['TDS_SUGAR_MIN'] <= TDS <= THRESHOLDS['TDS_SUGAR_MAX']:
        if TDS < 1280 and pH < 6.9:   # avoid false sugar on pure
            return {
                'status': 'PURE',
                'reason': f'TDS={TDS:.1f} near sugar range but pH={pH:.2f} < 6.9',
                'confidence': 0.82
            }
        return {
            'status': 'SUGAR',
            'reason': f'TDS={TDS:.1f} in sugar range',
            'confidence': 0.85
        }

    # 6. Water
    if TDS < THRESHOLDS['TDS_WATER_MAX']:
        return {
            'status': 'WATER',
            'reason': f'TDS={TDS:.1f} < {THRESHOLDS["TDS_WATER_MAX"]}',
            'confidence': 0.90
        }

    # 7. Pure (fallback)
    return {
        'status': 'PURE',
        'reason': 'all parameters within normal range',
        'confidence': 0.80
    }