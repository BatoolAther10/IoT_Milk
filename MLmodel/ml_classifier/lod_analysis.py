
def determine_lod(concentrations, hit_rates, threshold=0.90):
    """Return lowest concentration where hit_rate >= threshold."""
    for conc, rate in zip(concentrations, hit_rates):
        if rate >= threshold:
            return conc
    return None

if __name__ == "__main__":
    # Simulated data from Day 8 (replace with your actual measured values)
    lod_data = {
        'WATER':  ([0, 2, 5, 10, 15, 20], [0.0, 0.6, 1.0, 1.0, 1.0, 1.0]),
        'UREA':   ([0, 2, 5, 10, 15, 20], [0.0, 0.4, 0.9, 1.0, 1.0, 1.0]),
        'SALT':   ([0, 2, 5, 10, 15, 20], [0.0, 1.0, 1.0, 1.0, 1.0, 1.0]),
        'SUGAR':  ([0, 2, 5, 10, 15, 20], [0.0, 0.2, 0.6, 0.9, 1.0, 1.0]),
        'STARCH': ([0, 2, 5, 10, 15, 20], [0.0, 0.5, 0.95, 1.0, 1.0, 1.0]),
    }

    print("LIMIT OF DETECTION (LOD)\n" + "="*40)
    for adulterant, (concs, rates) in lod_data.items():
        lod = determine_lod(concs, rates)
        if lod is not None:
            print(f"{adulterant:>8} : LOD = {lod}%")
        else:
            print(f"{adulterant:>8} : LOD > max tested")