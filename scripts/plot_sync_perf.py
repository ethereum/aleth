#!/usr/bin/env python3

import json
import matplotlib.pyplot as plt
import sys

if len(sys.argv) < 3:
    print("USAGE: plot_sync_perf.py.py gas_per_sec|avg_gas_per_sec|avg_gas_per_sec_1000blocks|sync_time LOG_FILE")
    sys.exit(-1)

mode = sys.argv[1]
log_file = open(sys.argv[2], "r")

print("processing...")
perf_records = []
for line in log_file:
    if line.find("SLOW IMPORT:") == -1:
        continue

    perf_record = line[line.find("{"):line.find("}") + 1]
    # workaround to strip EthReset control sequence from the string
    perf_record = perf_record.replace("\x1b[0m", "")
    j = json.loads(perf_record)
    perf_records.append(j)

block_numbers = [r["blockNumber"] for r in perf_records]
gas_per_second = [r["gasPerSecond"] for r in perf_records]

print("plotting...")
if mode == "gas_per_sec":
    plt.plot(block_numbers, gas_per_second, ".")
    plt.xlabel("blockNumber")
    plt.ylabel("gasPerSecond")

elif mode == "avg_gas_per_sec":
    sum = 0
    avg_gps = []
    for i in range(0, len(gas_per_second)):
        sum += gas_per_second[i]
        avg_gps.append(sum / (i + 1))

    plt.plot(block_numbers, avg_gps)
    plt.xlabel("blockNumber")
    plt.ylabel("avg gasPerSecond")

elif mode == "avg_gas_per_sec_1000blocks":
    avg_gps = []
    sum1000 = 0
    for i in range(min(1000, len(gas_per_second))):
        sum1000 += gas_per_second[i]
        avg_gps.append(sum1000 / (i + 1))
        
    for i in range(1000, len(gas_per_second)):
        sum1000 += gas_per_second[i] - gas_per_second[i - 1000]
        avg_gps.append(sum1000 / 1000)

    plt.plot(block_numbers, avg_gps)
    plt.xlabel("blockNumber")
    plt.ylabel("avg 1000 blocks gasPerSecond")

elif mode == "sync_time":
    time = [r["total"] for r in perf_records]

    time_cum = time
    for i in range(1, len(time)):
      time_cum[i] += time_cum[i-1]

    time_cum_hours = [x / 3600 for x in time_cum]

    plt.plot(block_numbers, time_cum_hours)
    plt.xlabel("blockNumber")
    plt.ylabel("total sync time (h)")

else:
    print("Invalid mode")
    sys.exit(-1)


plt.show()
