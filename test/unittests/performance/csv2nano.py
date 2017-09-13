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
		gas = clients.pop(0)
		continue

	# read the data
	seconds = [second for second in line.rstrip().split(', ')]
	test = seconds.pop(0)
	gas = seconds.pop(0)
	if test not in tests:
		tests += [test]
	if test not in data:
		data[test] = {}
	for i, second in enumerate (seconds):
		data[test][clients[i]] = second

# print extended header
sys.stdout.write("ns/OP")
for client in clients:
	sys.stdout.write(", " + client)
sys.stdout.write("\n")

# nanos = ns/op is test secs offset by pop secs and scaled by number of operations
# print the test, secs, ..., nanos, ...
N_ops = 2**27
N_exp = 2**17
n = 0
for test in tests:
	if test == "nop":
		continue;
	sys.stdout.write(test)
	if test == "exp":
		n = N_exp
	else:
		n = N_ops
	for client in clients:
		pop_seconds = float(data["pop"][client])
		raw_seconds = float(data[test][client])
		op_seconds = raw_seconds - pop_seconds 
		sys.stdout.write(", %d" % int(op_seconds*10**9/n + .5))
	sys.stdout.write("\n")
