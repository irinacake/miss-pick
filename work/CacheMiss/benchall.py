import os, json
from pathlib import Path
import shlex
from subprocess import Popen, PIPE
from threading import Timer
import time

def run(cmd, timeout_sec):
    print(cmd)
    proc = Popen(shlex.split(cmd), stdout=PIPE, stderr=PIPE)
    timer = Timer(timeout_sec, proc.kill)
    try:
        timer.start()
        stdout, stderr = proc.communicate()
    finally:
        timer.cancel()


#folders = [Path("tacle-bench/bench/app"), Path("tacle-bench/bench/kernel"), Path("tacle-bench/bench/parallel"), Path("tacle-bench/bench/sequential")]
folders = [Path("tacle-bench/bench/kernel"), Path("tacle-bench/bench/sequential")]
#folders = []

banned = ["susan", "rosace"]
execpath = "./build/test"
timeout = 1200


resultdir = "./data/exp" + str(int(time.time())) + "/"
Path(resultdir).mkdir(parents=True, exist_ok=True)

for folder in folders:
    subfolders = [f for f in folder.glob('*')]
    for subfolder in subfolders:
        taskjson = os.path.join(subfolder,"TASKS.json")
        
        jsonfile = json.load(open(taskjson,'r'))
        tasks = jsonfile['tasks']
        execs = jsonfile['execs']

        for task in tasks:
            if 'exec' in task :
                execname = task['exec']
                for exec in execs:
                    if exec['name'] == execname:
                        subelf = exec['path']
                elf = os.path.join(subfolder,subelf)
                        
            else:
                elf = os.path.join(subfolder,os.path.basename(subfolder) + ".elf")

            if os.path.basename(subfolder) not in banned :
                print("\n----------------------------------------------------------")
                for way in [4,8]:
                    for row in [32,256]:
                        run(execpath + " " + elf + " " + task['name'] + " -c mycaches/mycacheFIFO" + str(row) + "_" + str(way) + ".xml -p --cfg-raw", timeout)
                        #run(execpath + " " + elf + " " + task['name'] + " -c mycaches/mycacheFIFO" + str(row) + "_" + str(way) + ".xml --dump-for CacheMissProcessor --dump-to " + resultdir + task['name'] + "_FIFO_" + str(row) + "_" + str(way) + ".json -p ", timeout)

                        run(execpath + " " + elf + " " + task['name'] + " -c mycaches/mycacheLRU" + str(row) + "_" + str(way) + ".xml -p --cfg-raw", timeout)

                        run(execpath + " " + elf + " " + task['name'] + " -c mycaches/mycachePLRU" + str(row) + "_" + str(way) + ".xml -p --cfg-raw", timeout)

                print("----------------------------------------------------------")


