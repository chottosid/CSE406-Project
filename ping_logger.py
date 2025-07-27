import time
from datetime import datetime, timedelta
import matplotlib.pyplot as plt
from ping3 import ping
import numpy as np

HOST = "google.com"
PING_INTERVAL = 1  # seconds
RUN_DURATION = 10  # seconds
MAX_POINTS = 100

times = []
rtts = []

plt.ion()
fig, ax = plt.subplots()
line, = ax.plot_date(times, rtts, '-o', label='Ping RTT (ms)')
ax.set_ylabel("Round-trip time (ms)")
ax.set_xlabel("Time")
ax.set_title(f"Ping RTT to {HOST}")
ax.legend()
fig.autofmt_xdate()

end_time = datetime.now() + timedelta(seconds=RUN_DURATION)

try:
    while datetime.now() < end_time:
        timestamp = datetime.now()
        try:
            result = ping(HOST, timeout=1)
        except Exception as e:
            print(f"Ping error: {e}")
            result = None

        rtt_ms = result * 1000 if result else None

        if rtt_ms is not None:
            print(f"{timestamp.strftime('%H:%M:%S')} - RTT: {rtt_ms:.2f} ms")
        else:
            print(f"{timestamp.strftime('%H:%M:%S')} - Request timed out")

        times.append(timestamp)
        rtts.append(rtt_ms if rtt_ms is not None else float('nan'))

        if len(times) > MAX_POINTS:
            times = times[-MAX_POINTS:]
            rtts = rtts[-MAX_POINTS:]

        line.set_data(times, rtts)
        ax.relim()
        ax.autoscale_view()
        plt.pause(0.01)

        time.sleep(PING_INTERVAL)

except KeyboardInterrupt:
    print("\nPing logging stopped by user.")

plt.ioff()

# Compute stats ignoring NaNs
rtts_np = np.array(rtts)
valid_rtts = rtts_np[~np.isnan(rtts_np)]

avg_rtt = valid_rtts.mean() if valid_rtts.size > 0 else float('nan')
max_rtt = valid_rtts.max() if valid_rtts.size > 0 else float('nan')
min_rtt = valid_rtts.min() if valid_rtts.size > 0 else float('nan')

stats_text = f"Avg RTT: {avg_rtt:.2f} ms\nMax RTT: {max_rtt:.2f} ms\nMin RTT: {min_rtt:.2f} ms"
print("\n" + stats_text)

# Show stats on plot
ax.text(0.02, 0.95, stats_text, transform=ax.transAxes, fontsize=10,
        verticalalignment='top', bbox=dict(boxstyle="round,pad=0.3", facecolor="yellow", alpha=0.3))

plt.draw()
plt.savefig("ping_plot.png")
plt.show()
