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
		units = clients.pop(0)
		continue

	# read the data
	seconds = [second for second in line.rstrip().split(', ')]
	test = seconds.pop(0)
	if test not in tests:
		tests += [test]
	if test not in data:
		data[test] = {}
	for i, second in enumerate (seconds):
		client = clients[i]
		data[test][client] = second

# print extended header
sys.stdout.write("(ns/test)")
for client in clients:
	sys.stdout.write(", " + client)
sys.stdout.write("\n")

# ns/test is run time scaled by number of operations
# print the test, gas, nanos, ...
N_ops = 2**27
N_app = 2**20
N = 0
for test in tests:
	sys.stdout.write(test)
	if test in ['nop', 'pop', 'add64', 'add128', 'add256', 'sub64', 'sub128', 'sub256',
	            'mul64', 'mul128', 'mul256', 'div64', 'div128', 'div256']:
		N = N_ops
	else:
		N = N_app
	for client in clients:
		if client == 'gas':
			gas_per_test = int(float(data[test][client])/N + .5)
			sys.stdout.write(", %d" % gas_per_test)
			continue
		nanos_per_test = int(float(data[test][client])*10**9/N + .5)
		sys.stdout.write(", %d" % nanos_per_test)
	sys.stdout.write("\n")
