from classifier import classify

if __name__ == "__main__":
    test_samples = [
        (6.7, 500,   5.0, 2.0, 0.02, "WATER"),
        (7.2, 800,   5.0, 2.0, 0.20, "UREA"),
        (6.7, 2200,  5.0, 2.0, 0.01, "SALT"),
        (6.7, 1500,  5.0, 2.0, 0.01, "SUGAR"),
        (6.7, 1000,  5.0, 12.0, 0.01, "STARCH"),
        (6.8, 900,   25.0, 2.0, 0.01, "SPOILED"),
        (6.8, 900,   5.0, 2.0, 0.01, "PURE"),
        (6.9, 1260,  5.0, 2.0, 0.01, "PURE"),   # hysteresis test
    ]

    print("DAY 3 VALIDATION\n" + "="*40)
    passed = 0
    for pH, TDS, NH3, dur, slope, expected in test_samples:
        res = classify(pH, TDS, NH3, dur, slope)
        ok = res['status'] == expected
        if ok: passed += 1
        print(f"{'✅' if ok else '❌'} Expected {expected:>7}, got {res['status']:>7} | {res['reason']}")

    print(f"\nPassed: {passed}/{len(test_samples)} ({passed/len(test_samples)*100:.1f}%)")