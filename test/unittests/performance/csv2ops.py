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
	if test not in ['nop', 'pop', 'add64', 'add128', 'add256', 'sub64', 'sub128', 'sub256',
	                'mul64', 'mul128', 'mul256', 'div64', 'div128', 'div256']:
		continue
	if test not in tests:
		tests += [test]
	if test not in data:
		data[test] = {}
	for i, second in enumerate (seconds):
		client = clients[i]
		data[test][client] = second

# print header
sys.stdout.write("(ns/OP)")
for client in clients:
	if client == 'gas':
		continue
	sys.stdout.write(", " + client)
sys.stdout.write("\n")

# ns/op is run time scaled by number of operations and offset by estimated overhead
# print the test, gas, nanos, ...
N = 2**27
for test in tests:
	if test in ['nop', 'pop']:
		continue
	sys.stdout.write(test)
	for client in clients:
		if client == 'gas':
			continue
		nanos_per_test = int(float(data[test][client])*10**9/N + .5)
		nanos_per_nop = int(float(data['nop'][client])*10**9/N + .5)
		nanos_per_pop = int(float(data['pop'][client])*10**9/N + .5)
		nanos_per_test -= (nanos_per_nop + nanos_per_pop)/2
		sys.stdout.write(", %d" % nanos_per_test)
	sys.stdout.write("\n")
