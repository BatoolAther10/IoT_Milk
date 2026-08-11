import csv
from collections import defaultdict
from thresholds_config import LABELS

def load_predictions(csv_file):
    """Read CSV, return lists of actual and predicted."""
    actual, pred = [], []
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            actual.append(row['Actual'])
            pred.append(row['Predicted'])
    return actual, pred

def compute_confusion_matrix(y_true, y_pred):
    cm = {l: {p: 0 for p in LABELS} for l in LABELS}
    for t, p in zip(y_true, y_pred):
        cm[t][p] = cm[t].get(p, 0) + 1
    return cm

def print_confusion_matrix(cm):
    print("\n" + "="*60)
    print("CONFUSION MATRIX")
    print("="*60)
    header = " " * 10 + "".join(f"{l:>10}" for l in LABELS)
    print(header)
    for true_l in LABELS:
        row = f"{true_l:<10} " + "".join(f"{cm[true_l].get(pred_l, 0):>10}" for pred_l in LABELS)
        print(row)
    print("="*60)

def compute_metrics(cm):
    metrics = {}
    total = sum(sum(row.values()) for row in cm.values())
    for label in LABELS:
        TP = cm[label].get(label, 0)
        FN = sum(cm[label].values()) - TP
        FP = sum(cm[other].get(label, 0) for other in LABELS if other != label)
        TN = total - (TP + FN + FP)

        sens = TP / (TP + FN) if (TP+FN) else 0.0
        spec = TN / (TN + FP) if (TN+FP) else 0.0
        prec = TP / (TP + FP) if (TP+FP) else 0.0
        f1 = 2 * (prec * sens) / (prec + sens) if (prec+sens) else 0.0
        metrics[label] = {'sensitivity': sens, 'specificity': spec, 'f1_score': f1}

    acc = sum(cm[l].get(l, 0) for l in LABELS) / total if total else 0.0
    metrics['overall_accuracy'] = acc
    return metrics

def print_metrics(metrics):
    print("\nPER-LABEL METRICS")
    print("-"*40)
    for label in LABELS:
        m = metrics[label]
        print(f"{label:>8} | Sens: {m['sensitivity']:.3f}  Spec: {m['specificity']:.3f}  F1: {m['f1_score']:.3f}")
    print("-"*40)
    print(f"Overall Accuracy: {metrics['overall_accuracy']:.3f} ({metrics['overall_accuracy']*100:.1f}%)")
    print("="*60)

if __name__ == "__main__":
    y_true, y_pred = load_predictions('test_results_day6.csv')
    cm = compute_confusion_matrix(y_true, y_pred)
    print_confusion_matrix(cm)
    metrics = compute_metrics(cm)
    print_metrics(metrics)