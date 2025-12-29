#!/usr/bin/python3
import subprocess
import re
import statistics
import math
import sys
import platform
import os.path
import psutil

def erf_inv(x):
	"""Inverse of math.erf()"""
	a = 8 * (math.pi - 3) / (3 * math.pi*(4 - math.pi))
	y = math.log(1 - x * x)
	z = 2 / (math.pi * a) + y / 2
	return math.copysign(math.sqrt(math.sqrt(z * z - y / a) - z), x)

def CDF(q):
	"""CDF of the standard Gaussian law"""
	return 0.5 * (1 + math.erf(q / math.sqrt(2)))

def Quantile(p):
	"""Quantile function of the standard Gaussian law"""
	assert(0 <= p and p <= 1)
	return math.sqrt(2)*erf_inv(2*p-1)

def rightstr(s, l):
	if len(s) > l:
		return "..."+s[3-l:]
	else:
		return s

if (len(sys.argv) != 5 or int(sys.argv[3]) < 2):
	exit("Usage:\n" + sys.argv[0] + " base test #min_runs #max_runs\nwhere #min_runs >= 2")

base, test, diff = [], [], []
min_runs = int(sys.argv[3])
max_runs = int(sys.argv[4])
exp = re.compile(br'(\d+) pps')

# determine CPU sets to run on
# this assumes that logical cpus on the same core are numbered sequentially
num_cores = psutil.cpu_count(logical=False)
num_cpus = psutil.cpu_count()
cpu_step = int(num_cpus / num_cores)
cpuset = []
cpuset.append(list())
cpuset.append(list())
for i in range(0, cpu_step):
	cpuset[0].extend(list(range(i, num_cpus, cpu_step*2)))
	cpuset[1].extend(list(range(i+cpu_step, num_cpus, cpu_step*2)))
print("{:>3} {:>10} {:>10} {:>8}".format("run","base","test","diff"))

Q = Quantile(0.975)
runs = 0
for i in range(max_runs):
	# Start both processes. This is non-blocking.
	if i & 1:
		base_process = subprocess.Popen([sys.argv[1],"1"], stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
		psutil.Process(base_process.pid).cpu_affinity(cpuset[i%2])
		test_process = subprocess.Popen([sys.argv[2],"1"], stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
		psutil.Process(test_process.pid).cpu_affinity(cpuset[(i+1)%2])
	else:
		test_process = subprocess.Popen([sys.argv[2],"1"], stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
		psutil.Process(test_process.pid).cpu_affinity(cpuset[i%2])
		base_process = subprocess.Popen([sys.argv[1],"1"], stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
		psutil.Process(base_process.pid).cpu_affinity(cpuset[(i+1)%2])

	# Wait for processes to finish and grep nps results in their stderr output
	base.append(int(exp.search(base_process.stderr.read()[-30:]).group(1)))
	test.append(int(exp.search(test_process.stderr.read()[-30:]).group(1)))

	diff.append(test[i] - base[i])

	runs += 1
	if i > 0:
		diff_mean = statistics.mean(diff)
		diff_sdev = statistics.stdev(diff, diff_mean) / math.sqrt(runs)
		print("{:>3} {:>10n} {:>10n} {:>+8n} {:>+10n} +/- {:n}".format (i + 1, base[i], test[i], diff[i], round(diff_mean), round(Q * diff_sdev)))
		if runs >= min_runs and abs(diff_mean) > Q * diff_sdev: break
	else:
		print("{:>3} {:>10n} {:>10n} {:>+8n}".format (i + 1, base[i], test[i], diff[i]))

base_mean = statistics.mean(base)
test_mean = statistics.mean(test)
diff_mean = statistics.mean(diff)

base_sdev = statistics.stdev(base, base_mean) / math.sqrt(runs)
test_sdev = statistics.stdev(test, test_mean) / math.sqrt(runs)
diff_sdev = statistics.stdev(diff, diff_mean) / math.sqrt(runs)
print("\nResult of {:3n} runs\n==================".format(runs))
print("base ({:<15s}) = {:>10n}  +/- {:n}".format(rightstr(sys.argv[1],15), round(base_mean), round(Quantile(0.975) * base_sdev)))
print("test ({:<15s}) = {:>10n}  +/- {:n}".format(rightstr(sys.argv[2],15), round(test_mean), round(Quantile(0.975) * test_sdev)))
print("{:22s} = {:>+10n}  +/- {:n}".format("diff", round(diff_mean), round(Q * diff_sdev)))
print("\nspeedup        = {:>+6.4f}  +/- {:>6.4f}".format(diff_mean / base_mean, Q * diff_sdev / base_mean))
print("P(speedup > 0) = {:>7.4f}".format(CDF(diff_mean / diff_sdev)))

cpu = str()
if os.path.exists("/proc/cpuinfo"):
	exp = re.compile (r"^model name\s+:\s+(.*)$")
	with open('/proc/cpuinfo','r') as infile:
		for line in infile:
			m = exp.match(line)
			if m:
				cpu = m.group(1)
				break
if cpu == "":
	cpu = platform.processor()
if cpu == "":
	cpu = "unknown"
print("\nCPU:",num_cores,"x",cpu)
print("Hyperthreading:", "off" if num_cpus==num_cores else "on","\n")
