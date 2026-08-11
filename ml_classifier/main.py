import subprocess
import os

scripts = [
    'validation.py',
    'batch_tests.py',
    'stats_analysis.py',
    'lod_analysis.py'
]

print("M1 – RUNNING ALL CODE MODULES\n" + "="*60)
for script in scripts:
    print(f"\n>>> Executing {script} ...")
    subprocess.run(['python', script])
    print("-"*60)