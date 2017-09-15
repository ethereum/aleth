import sys
import re

clients = []
seconds = []
tests = []
data = {}

# read in csv file
for line in sys.stdin:

	# read the header
	if not clients:
		clients = [word for word in line.rstrip().split(', ')]
		clients.pop(0)
		continue

	# read the data
	seconds = [second for second in line.rstrip().split(', ')]
	test = seconds.pop(0)
	if test not in tests:
		tests += [test]
	if test not in data:
		data[test] = {}
	for i, second in enumerate (seconds):
		data[test][clients[i]] = second

# print extended header
sys.stdout.write("(ns/gas)")
for client in clients:
	if client == 'gas/run':
		continue
	sys.stdout.write(", " + client)
sys.stdout.write("\n")

# nanos = ns/gas is run time scaled by amount of gas
# print the test, gas, nanos ...
N_ops = 2**27
N_exp = 2**20
for test in tests:
	sys.stdout.write(test)
	gas = float(data[test]['gas/run'])
	for client in clients:
		if client == 'gas/run':
			continue
		run_nanos = float(data[test][client])*10**9
		nanos_per_gas = int(run_nanos/gas + .5)
		sys.stdout.write(", %d" % nanos_per_gas)
	sys.stdout.write("\n")
