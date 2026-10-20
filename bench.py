#!/usr/bin/env python3
"""Compare deterministic benches: python3 bench.py build/pz-base ./pzchessbot 10."""
import argparse
import math
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess


PZ_RESULT = re.compile(r"^(\d+) nodes (\d+) nps\s*$", re.MULTILINE)
SF_NODES = re.compile(r"Nodes searched\s*:\s*(\d+)")
SF_NPS = re.compile(r"Nodes/second\s*:\s*(\d+)")


def parse_result(output):
	match = PZ_RESULT.search(output)
	if match:
		return tuple(map(int, match.groups()))
	nodes, nps = SF_NODES.search(output), SF_NPS.search(output)
	if nodes and nps:
		return int(nodes[1]), int(nps[1])
	raise ValueError("No benchmark summary found in engine output:\n" + output[-2000:])


def run_bench(engine, depth):
	command = [str(Path(engine).resolve()), "bench"]
	if depth is not None:
		command.append(str(depth))
	result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
		text=True, check=True, timeout=600)
	return parse_result(result.stdout)


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("base")
	parser.add_argument("test")
	parser.add_argument("runs", type=int, help="number of paired runs (at least 2)")
	parser.add_argument("--depth", type=int, help="override PZChessBot bench depth")
	parser.add_argument("--cpu", type=int, help="pin both engines to this logical CPU (Linux)")
	args = parser.parse_args()
	if args.runs < 2:
		parser.error("runs must be at least 2")
	if args.depth is not None and args.depth < 1:
		parser.error("depth must be positive")

	if hasattr(os, "sched_getaffinity"):
		allowed = os.sched_getaffinity(0)
		cpu = min(allowed) if args.cpu is None else args.cpu
		if cpu not in allowed:
			parser.error(f"CPU {cpu} is outside the allowed affinity {sorted(allowed)}")
		# Children inherit affinity, including their search threads. Run sequentially
		# on the same core to avoid SMT and shared-cache contention between engines.
		os.sched_setaffinity(0, {cpu})
		print(f"CPU: {cpu}; {platform.processor() or platform.machine()}")
	elif args.cpu is not None:
		parser.error("CPU pinning is not supported on this platform")

	print("Sequential paired runs, alternating base/test order; warming up each engine.", flush=True)
	try:
		reference, _ = run_bench(args.base, args.depth)
		test_nodes, _ = run_bench(args.test, args.depth)
		if reference != test_nodes:
			raise ValueError(f"Bench mismatch: base {reference} nodes, test {test_nodes} nodes")
		print(f"Bench: {reference} nodes")
		print(f"{'run':>3} {'base nps':>12} {'test nps':>12} {'speedup':>10}", flush=True)
		base, test, ratios = [], [], []
		for i in range(args.runs):
			results = {}
			order = ("base", "test") if i % 2 == 0 else ("test", "base")
			for name in order:
				nodes, nps = run_bench(getattr(args, name), args.depth)
				if nodes != reference or nps <= 0:
					raise ValueError(f"Invalid {name} bench: {nodes} nodes, {nps} nps; expected {reference} nodes")
				results[name] = nps
			base.append(results["base"])
			test.append(results["test"])
			ratio = results["test"] / results["base"]
			ratios.append(math.log(ratio))
			print(f"{i + 1:3} {base[-1]:12,d} {test[-1]:12,d} {ratio - 1:+9.2%}", flush=True)
	except (ValueError, OSError, subprocess.SubprocessError) as error:
		parser.exit(1, f"Benchmark failed: {error}\n")

	# Log ratios treat equal proportional gains/losses symmetrically.
	mean = statistics.mean(ratios)
	speedup = math.expm1(mean)
	margin = 1.96 * statistics.stdev(ratios) / math.sqrt(args.runs)
	lower, upper = math.expm1(mean - margin), math.expm1(mean + margin)
	print(f"\nMean base: {statistics.mean(base):,.0f} nps")
	print(f"Mean test: {statistics.mean(test):,.0f} nps")
	print(f"Paired geometric speedup: {speedup:+.2%}")
	print(f"Approximate 95% normal interval: {lower:+.2%} to {upper:+.2%}")


if __name__ == "__main__":
	main()
