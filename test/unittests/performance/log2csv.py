import sys
import re

# regex for the lines with relevant data
test_line = 'time -p ([-_./A-Za-z0-9]*/)*([-_A-Za-z0-9]+) .*; touch (.+).ran$'
gas_line = 'as used: ([0-9]+)'
secs_line = 'user ([0-9.]+)'

# read in data
path = ''
client = ''
test = ''
clients = []
tests = []
gas = {}
data = {}
for line in sys.stdin:
	match = re.search(test_line, line)
	if match:
		path = match.group(1)
		client = match.group(2)
		test = match.group(3)
		if client not in clients:
			clients += [client]
		if test not in tests:
			tests += [test]
		continue
	match = re.search(gas_line, line)
	if match:
		gas[test] = match.group(1)
		continue
	match = re.search(secs_line, line)
	if match:
		if test not in data:
			data[test] = {}
		data[test][client] = match.group(1)
		continue

# print the header
sys.stdout.write("(sec/run)")
if len(gas):
	sys.stdout.write(", gas")
for client in clients:
	sys.stdout.write(", %s" % client)
sys.stdout.write("\n")

# print the test, gas, secs, secs, ...
for test in tests:
	sys.stdout.write("%s" % test)
	if gas[test]:
		sys.stdout.write(", %s" % gas[test])
	for client in clients:
		sys.stdout.write(", %s" % data[test][client])
	sys.stdout.write("\n")
