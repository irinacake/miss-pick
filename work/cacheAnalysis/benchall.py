import os, sys, json
from pathlib import Path
from time import sleep




#folders = [Path("tacle-bench/bench/app"), Path("tacle-bench/bench/kernel"), Path("tacle-bench/bench/parallel"), Path("tacle-bench/bench/sequential")]
folders = [Path("tacle-bench/bench/kernel")]
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
                print("./bin/cacheAnalysis " + elf + " " + task['name'] + " -c mycacheLRU.xml")
                print()
                os.system("./bin/cacheAnalysis " + elf + " " + task['name'] + " -c mycacheLRU.xml" )
                print("----------------------------------------------------------")
        


        #print(taskjson)


#def line_prepender(filename, line):
#    with open(filename, 'r+') as f:


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



