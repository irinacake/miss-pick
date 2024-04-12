#!/usr/bin/env bash

# Courtines Elana
# Institut de Recherche en Informatique de Toulouse (IRIT)
# April 2024
# M2 Internship


launch=`pwd`

folders="tacle-bench/bench/kernel/* tacle-bench/bench/sequential/* tacle-bench/bench/app/* tacle-bench/bench/parallel/*"


for algorithm in $folders
do
  if [ -d "$algorithm" ]
  then
    if [ -f "$algorithm/`basename $algorithm`.elf" ]
    then
        elf="$algorithm/`basename $algorithm`.elf"

        if [ `basename $algorithm` != "susan" ]
        then

          echo "-------------executing $elf------------------\n"

          funcs=`python3 -c "import sys, json; path = \"$algorithm/TASKS.json\";file = open(path, 'r');tasks = json.load(file)['tasks'];exec(\"for task in tasks: print(task['name'],end=' ')\")"`
        
          for func in $funcs
          do
            ./bin/cacheAnalysis $elf $func -c mycacheLRU.xml 
          done
          

          echo "------------------Done------------------\n"

        fi
    else
        echo "N/A"
    fi
  fi
done



#for algorithm in tacle-bench/bench/sequential/*
#do
#  if [ -d "$algorithm" ]
#  then
#    if [ -f "$algorithm/`basename $algorithm`.elf" ]
#    then
#        elf="$algorithm/`basename $algorithm`.elf"
#        #./bin/cacheAnalysis $elf main -c mycacheLRU.xml 
#        python3 -c "import sys, json; path = \"$algorithm/TASKS.json\"; file = open(path, 'r'); print(json.load(file)['tasks'][0]['name'])"
#    else
#        echo "N/A"
#    fi
#  fi
#done
