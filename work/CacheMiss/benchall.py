import os, json
from pathlib import Path
import shlex
from subprocess import Popen, PIPE
import subprocess
from threading import Timer
import time
import sys
import math

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

not_working = ["susan", "rosace"]
no_ff = ["ammunition", "rijndael_enc", "huff_enc", "anagram", "quicksort", "bitonic", "recursion", "gsm_enc", "pm"]
too_slow = ["epic", "mpeg2", "petrinet", "rijndael_dec", "audiobeam", "gsm_enc", "gsm_dec", "quicksort", "fmref", "sha", "cubic", "md5", "pm", "statemate","g723_enc"]
#banned = ["bitonic", "recursion", "audiobeam", "gsm_enc", "gsm_dec", "quicksort", "fmref", "sha", "cubic", "md5", "pm", "susan", "rosace"]
banned = not_working + no_ff# + too_slow
execpath = "./build/test"
timeout = 3600



cmd_master_list = []

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
                result = subprocess.run(['ostat', '-g', elf], stdout=subprocess.PIPE)
                resultstr = result.stdout.decode('utf-8')
                codesize = int(resultstr.split(":")[-1])
                for t in [codesize]:#,0.5*codesize,0.25*codesize]:
                    l = math.ceil(math.log2(t/64))
                    print("L:", l)
                    if (l < 1 or l > 8):
                        break

                    cmd = [execpath, elf, task['name'], "-c", "mycaches/4/mycacheFIFO" + str(l) + ".xml", "-p", "--cfg-raw", "-o", resultdir + task['name'] + "_FIFO_" + str(l) + "_4.json"]
                    print("- Adding ", elf)
                    cmd_master_list.append(cmd)
                    #subprocess.Popen(cmd)

                    cmd = [execpath, elf, task['name'], "-c", "mycaches/4/mycacheLRU" + str(l) + ".xml", "-p", "--cfg-raw", "-o", resultdir + task['name'] + "_LRU_" + str(l) + "_4.json"]
                    print("- Adding ", elf)
                    cmd_master_list.append(cmd)
                    #subprocess.Popen(cmd)

                    cmd = [execpath, elf, task['name'], "-c", "mycaches/4/mycachePLRU" + str(l) + ".xml", "-p", "--cfg-raw", "-o", resultdir + task['name'] + "_PLRU_" + str(l) + "_4.json"]
                    print("- Adding ", elf)
                    cmd_master_list.append(cmd)
                    #subprocess.Popen(cmd)


                print("----------------------------------------------------------")


cpt = len(cmd_master_list)
running_p = []
for cmd in cmd_master_list:
    while len(running_p) > 15:
        for p in running_p:
            if p.poll() is not None:
                print("finished: ", " ".join(p.args))
                running_p.remove(p)
                break
    print("launched: ", " ".join(cmd))
    p = subprocess.Popen(cmd)
    cpt-=1
    print("Processes waiting: ", cpt)
    
    running_p.append(p)



while len(running_p) > 0:
    for p in running_p:
        if p.poll() is not None:
            print("finished: ", " ".join(p.args))
            running_p.remove(p)
            print("Processes ongoing left: ", len(running_p))
            break
        