"""Season simulator for the ESP32 irrigation controller (same decision logic as the firmware)."""
import argparse
import random
import statistics

STAGES = [("seedling", 0, 4, 45), ("vegetative", 5, 9, 38), ("flowering", 10, 13, 42)]  # name, day from, day to, min moisture %
TARGET_OFFSET = 12      # water up to threshold + 12 points
MIN_PER_POINT = 0.8     # minutes of watering per moisture point
MAX_RUN_MIN = 20
RAIN_SKIP_MM = 5


def threshold(day):
    return next(t for name, a, b, t in STAGES if a <= day <= b)


def decide(readings, day, forecast_mm, override):
    """Returns (valve_minutes, reason). Mirrors decide() in irrigation_controller.ino."""
    if override == "OFF":
        return 0, "manual override OFF"
    if override == "ON":
        return MAX_RUN_MIN, "manual override ON"
    m = statistics.median(readings)
    th = threshold(day)
    if m >= th:
        return 0, f"moist enough ({m:.0f}% >= {th}%)"
    if forecast_mm >= RAIN_SKIP_MM:
        return 0, f"dry ({m:.0f}%) but {forecast_mm} mm rain forecast"
    minutes = min(MAX_RUN_MIN, round((th + TARGET_OFFSET - m) * MIN_PER_POINT))
    return minutes, f"dry ({m:.0f}% < {th}%), watering {minutes} min"


def simulate(seed=4, days=14):
    rng = random.Random(seed)
    soil = 50.0
    log, water = [], 0
    for day in range(days):
        rain = rng.choice([0, 0, 0, 0, 2, 12, 25])
        forecast = rain if rng.random() < 0.8 else 0
        soil = min(70, soil - rng.uniform(4, 7) + rain * 0.8)
        readings = [soil + rng.gauss(0, 1.5), soil + rng.gauss(0, 1.5), soil + (25 if day == 6 else rng.gauss(0, 1.5))]
        override = "OFF" if day == 11 else None
        mins, why = decide(readings, day, forecast, override)
        soil = min(70, soil + mins / MIN_PER_POINT)
        water += mins
        log.append((day, [round(r) for r in readings], forecast, rain, mins, why))
    return log, water


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--demo", action="store_true")
    ap.add_argument("--seed", type=int, default=4)
    a = ap.parse_args()
    log, water = simulate(a.seed)
    print("Day  Sensors (%)      Forecast  Rain  Valve  Decision")
    for day, r, fc, rain, mins, why in log:
        print(f"{day:>3}  {str(r):<16} {fc:>5} mm {rain:>3} mm {mins:>3} min  {why}")
    print(f"\nTotal valve time: {water} min over {len(log)} days "
          f"(a fixed 15 min/day timer would use {15 * len(log)} min)")


if __name__ == "__main__":
    main()
