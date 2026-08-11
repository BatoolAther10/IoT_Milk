import csv
from classifier import classify

# ---------- 18-test dataset (Day 5) ----------
day5_data = [
    # 3 runs per sample
    (6.7, 980,   8.0, 2.0, 0.02, "PURE"),
    (6.8, 1010,  7.0, 2.3, 0.01, "PURE"),
    (6.6, 970,   9.0, 1.9, 0.02, "PURE"),
    (6.7, 480,   5.0, 1.8, 0.01, "WATER"),
    (6.6, 510,   6.0, 2.0, 0.01, "WATER"),
    (6.8, 490,   5.0, 2.2, 0.01, "WATER"),
    (7.3, 850,   12.0, 1.5, 0.22, "UREA"),
    (7.2, 830,   11.0, 1.6, 0.19, "UREA"),
    (7.4, 860,   13.0, 1.4, 0.25, "UREA"),
    (6.5, 2350,  6.0, 2.0, 0.01, "SALT"),
    (6.6, 2400,  7.0, 1.9, 0.01, "SALT"),
    (6.5, 2280,  6.0, 2.1, 0.01, "SALT"),
    (6.7, 1420,  8.0, 2.0, 0.01, "SUGAR"),
    (6.8, 1380,  7.0, 1.8, 0.01, "SUGAR"),
    (6.7, 1450,  8.0, 2.2, 0.01, "SUGAR"),
    (6.6, 1050,  7.0, 11.5, 0.01, "STARCH"),
    (6.7, 1080,  8.0, 12.0, 0.01, "STARCH"),
    (6.5, 1020,  7.0, 11.0, 0.01, "STARCH"),
]

# ---------- 30-test dataset (Day 6) – 5 runs each ----------
day6_data = []
for _ in range(5):
    day6_data.extend([
        (6.7, 980,   8.0, 2.0, 0.02, "PURE"),
        (6.7, 490,   5.0, 2.0, 0.01, "WATER"),
        (7.3, 850,   12.0, 2.0, 0.22, "UREA"),
        (6.5, 2350,  6.0, 2.0, 0.01, "SALT"),
        (6.7, 1420,  8.0, 2.0, 0.01, "SUGAR"),
        (6.6, 1050,  7.0, 11.5, 0.01, "STARCH"),
        (6.8, 900,   25.0, 2.0, 0.01, "SPOILED"),
    ])
# Introduce the known Day-6 error (pure → sugar)
day6_data[3] = (6.8, 1260, 8.0, 2.0, 0.02, "PURE")   # will misclassify as sugar

def run_batch(data, filename):
    """Run classifier on a batch, write results to CSV."""
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['pH','TDS','NH3','StableDur','Slope','Actual','Predicted','Confidence','Correct'])
        correct = 0
        for row in data:
            pH, TDS, NH3, dur, slope, actual = row
            res = classify(pH, TDS, NH3, dur, slope)
            pred = res['status']
            conf = res['confidence']
            correct_flag = (pred == actual)
            if correct_flag: correct += 1
            writer.writerow([pH, TDS, NH3, dur, slope, actual, pred, f"{conf:.3f}", correct_flag])
        return correct, len(data)

if __name__ == "__main__":
    print("Running Day 5 batch (18 tests)...")
    c5, t5 = run_batch(day5_data, 'test_results_day5.csv')
    print(f"Day 5 Accuracy: {c5}/{t5} = {c5/t5*100:.1f}%")

    print("\nRunning Day 6 batch (30 tests)...")
    c6, t6 = run_batch(day6_data, 'test_results_day6.csv')
    print(f"Day 6 Accuracy: {c6}/{t6} = {c6/t6*100:.1f}%")
