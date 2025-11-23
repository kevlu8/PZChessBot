import requests
import time
import subprocess
import cpuinfo

api = "https://pgn.int0x80.ca/api"

prev_netfile = "None"

ncores = subprocess.check_output(["nproc"]).decode().strip()

cpu = cpuinfo.get_cpu_info()['brand_raw']

def run_loop():
	while True:
		# check config and modify things if needed
		try:
			resp = requests.get(api)
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
		subprocess.run(["make", f"SNODES={snodes}", f"NRAND={nrand}"])
		# run engine
		print("Starting data generation...")
		# n cores
		print(f"Using {ncores} cores.")
		subprocess.run(["./pzchessbot", ncores], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		requests.post(f"{api}/report", json={"id": cpu, "games": 50000 * int(ncores)})

def keepalive():
	while True:
		try:
			requests.post(f"{api}/keepalive", json={"id": cpu, "cores": ncores})
		except:
			pass
		time.sleep(300)