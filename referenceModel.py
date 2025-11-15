import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import make_interp_spline
from pathlib import Path
import csv

# Function to simulate a more subtle delayed response in air flow
def subtle_delayed_response(speed_array, total_time, delay_seconds=10):
    time_resolution = len(speed_array) / total_time  # Points per minute
    delay_steps = int(delay_seconds * time_resolution / 60)  # Convert seconds delay to steps
    delayed_array = np.zeros_like(speed_array)
    for i in range(delay_steps, len(speed_array)):
        delayed_array[i] = speed_array[i - delay_steps]
    return delayed_array

# CSV path (same folder as this script) - assume series.csv already exists
csv_path = Path(__file__).parent / 'series.csv'
if not csv_path.exists():
    raise FileNotFoundError(f"series.csv not found at {csv_path}")

# Read CSV
times = []
humidities = []
fan_speeds = []
manual_overrides = []

with csv_path.open('r', newline='') as f:
    reader = csv.DictReader(f)
    for row in reader:
        if not row or all((v is None or str(v).strip() == '') for v in row.values()):
            continue
        times.append(float(row['time']))
        humidities.append(float(row['humidity']))
        fan_speeds.append(int(float(row['fan_speed'])))
        manual_overrides.append(int(float(row['manual_override'])))

# Convert to numpy arrays
time_120min = np.array(times, dtype=float)
humidity = np.array(humidities, dtype=float)
fan_speed = np.array(fan_speeds, dtype=int)
manual_override = np.array(manual_overrides, dtype=int)

# Re-apply manual override logic (in case CSV contains override flags)
for i in range(len(fan_speed) - 1):
    if manual_override[i] == 1:
        fan_speed[i] = 2
        fan_speed[i + 1] = 2

# Total time in minutes and delay in seconds
total_time_minutes = 120
delay_in_seconds = 10

# Apply subtle delay to fan speed to calculate air flow
subtle_delayed_fan_speed = subtle_delayed_response(fan_speed, total_time_minutes, delay_in_seconds)
subtle_air_flow_delayed = np.array([780 if speed == 1 else 1967 if speed == 2 else 450 for speed in subtle_delayed_fan_speed])

# Generate a smooth curve for humidity
X_Y_Spline = make_interp_spline(time_120min, humidity)
X_ = np.linspace(time_120min.min(), time_120min.max(), 500)
Y_ = X_Y_Spline(X_)

# Create subplots
fig, (ax1, ax2, ax3, ax4) = plt.subplots(4, 1, figsize=(10, 8), sharex=True)

# Plot Humidity
ax1.plot(X_, Y_, color='tab:blue')
ax1.set_ylabel('Humidity')
ax1.grid(True)

# Plot Fan Speed
ax2.step(time_120min, fan_speed, where='post', color='tab:red', linestyle='--')
ax2.set_ylabel('Fan Speed')
ax2.grid(True)

# Plot Subtle Delayed Air Flow
ax3.step(time_120min, subtle_air_flow_delayed, where='post', color='tab:green', linestyle='--')
ax3.set_ylabel('Air Flow (Subtle Delay)')
ax3.grid(True)

# Plot Manual Override
ax4.step(time_120min, manual_override, where='post', color='tab:purple', linestyle='--')
ax4.set_ylabel('Manual Override')
ax4.grid(True)

# Set x-axis label
ax4.set_xlabel('Time (minutes)')

# Adjust layout
plt.tight_layout()
plt.show()