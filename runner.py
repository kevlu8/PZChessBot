import requests
import time
import subprocess
import cpuinfo
from random_username.generate import generate_username

api = "https://pgn.int0x80.ca/api"

ncores = subprocess.check_output(["nproc"]).decode().strip()

cpu = cpuinfo.get_cpu_info()['brand_raw']

workerid = generate_username(1)[0]

def run_loop():
	global ncores, cpu, workerid, api
	prev_netfile = "None"
	while True:
		# check config and modify things if needed
		print("Fetching config...")
		try:
			resp = requests.get(f"{api}/config")
			if resp.status_code == 200:
				config = resp.json()
		except Exception as e:
			print(f"Error fetching config: {e}")
			time.sleep(60)
			continue
		nrand = config.get("num_rand", 12)
		snodes = config.get("soft_nodes", 5000)
		netfile = config.get("netfile", "None")
		if netfile != prev_netfile:
			print("Detected new network file.")
			# fetch
			try:
				nf_resp = requests.get(f"{api}/{netfile}")
				if nf_resp.status_code == 200:
					with open("nnue.bin", "wb") as f:
						f.write(nf_resp.content)
					print("Updated neural network file.")
					prev_netfile = netfile
			except Exception as e:
				print(f"Error fetching netfile: {e}")
				continue
		# re-compile
		subprocess.run(["make", "clean"])
		subprocess.run(["make", f"SNODES={snodes}", f"NRAND={nrand}", "-j4"])
		# run engine
		print("Starting data generation...")
		# n cores
		print(f"Using {ncores} cores.")
		result = subprocess.Popen(["./pzchessbot", str(ncores)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
		captured_output = []
		for line in result.stdout:
			print(line, end='')  # Print each line as it is received
			captured_output.append(line)
		result.stdout.close()
		result.wait()
		output = ''.join(captured_output)
		# parse output for total games and positions
		pos = output.split("Total positions: ")[1].split("\n")[0]
		requests.post(f"{api}/report/{workerid}", json={"cpu": cpu, "games": 500 * int(ncores), "positions": int(pos)})
		print("Reporting...")

def heartbeat():
	while True:
		try:
			requests.post(f"{api}/heartbeat/{workerid}", json={"cores": ncores, "cpu": cpu})
		except:
			pass
		time.sleep(10)

import threading
hb_thread = threading.Thread(target=heartbeat, daemon=True)
hb_thread.start()
run_loop()
