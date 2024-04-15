import os, json
from pathlib import Path
import shlex
from subprocess import Popen, PIPE
from threading import Timer

def run(cmd, timeout_sec):
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


result = open("results.json",'w')
result.close()

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
                        
            else :
                elf = os.path.join(subfolder,os.path.basename(subfolder) + ".elf")

            if os.path.basename(subfolder) not in banned :
                print("\n----------------------------------------------------------")
                for way in [4,8]:
                    for row in [32,256]:
                        print("./bin/cacheAnalysis " + elf + " "
                              + task['name'] + " -c mycaches/mycacheFIFO" + str(row) + "_" + str(way) + ".xml" )
                        run("./bin/cacheAnalysis " + elf + " "
                                  + task['name'] + " -c mycaches/mycacheFIFO" + str(row) + "_" + str(way) + ".xml", 360)
                        
                        print("./bin/cacheAnalysis " + elf + " "
                              + task['name'] + " -c mycaches/mycacheLRU" + str(row) + "_" + str(way) + ".xml" )
                        run("./bin/cacheAnalysis " + elf + " "
                                  + task['name'] + " -c mycaches/mycacheLRU" + str(row) + "_" + str(way) + ".xml", 360)
                        
                        print("./bin/cacheAnalysis " + elf + " "
                              + task['name'] + " -c mycaches/mycachePLRU" + str(row) + "_" + str(way) + ".xml" )
                        run("./bin/cacheAnalysis " + elf + " "
                                  + task['name'] + " -c mycaches/mycachePLRU" + str(row) + "_" + str(way) + ".xml", 360)
                print("----------------------------------------------------------")



result = open("results.json",'r+')
prejson = "{\n\t\"exp\": [\n"
content = result.read()
result.seek(0, 0)
result.write(prejson.rstrip('\r\n') + '\n' + content)
result.close()

result = open("results.json",'rb+')
result.seek(-2, os.SEEK_END)
result.truncate()
result.close()

result = open("results.json",'a')
result.seek(0,2)
result.write("\n\t]\n}\n")



