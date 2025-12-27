import requests
import time
import subprocess
import os
import cpuinfo
from random_username.generate import generate_username

api = "https://pgn.int0x80.ca/api"

ncores = min(64, int(subprocess.check_output(["nproc"]).decode().strip()))

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
				nf_resp = requests.get(f"{api}/net/{netfile}")
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
		prev_pos, prev_games = 0, 0
		for line in result.stdout:
			print(f"[./pzchessbot]: {line}", end='')  # Print each line as it is received
			captured_output.append(line)
			# Positions: x, Time: xs, PPS: x(float), Games: x
			if "Positions: " in line:
				positions = line.split("Positions: ")[1].split(",")[0]
				games = line.split("Games: ")[1].strip()
				pps = line.split("PPS: ")[1].split(",")[0]
				requests.post(f"{api}/report/{workerid}", json={"cpu": cpu, "positions": int(positions)-prev_pos, "games": int(games)-prev_games, "pps": int(round(float(pps)))})
				prev_pos = int(positions)
				prev_games = int(games)
		result.stdout.close()
		result.wait()
		# finished run, upload data
		# files will be of name "<id>_datagen.pgn"
		for i in range(int(ncores)):
			filename = f"{i}_datagen.pgn"
			# verify exists
			try:
				with open(filename, "rb") as f:
					pass
			except FileNotFoundError:
				print(f"Data file {filename} not found, skipping upload.")
				continue
			# compress
			print(f"Compressing and uploading {filename}...")
			subprocess.run(["zstd", "-19", "-q", filename])
			try:
				with open(filename + ".zst", "rb") as f:
					try:
						resp = requests.post(f"{api}/data", headers={"X-Keyword": "OpenBench"}, data=f)
						if resp.status_code == 200:
							print(f"Successfully uploaded {filename}.")
						else:
							print(f"Failed to upload {filename}, status code: {resp.status_code}")
					except Exception as e:
						print(f"Error uploading {filename}: {e}")
			except FileNotFoundError:
				print(f"Compressed file {filename}.zst not found, skipping upload.")
				continue
			# delete files
			os.remove(filename)
			os.remove(filename + ".zst")

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