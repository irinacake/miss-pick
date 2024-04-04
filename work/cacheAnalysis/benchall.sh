#!/usr/bin/env bash

# Courtines Elana
# Institut de Recherche en Informatique de Toulouse (IRIT)
# April 2024
# M2 Internship


launch=`pwd`

for algorithm in tacle-bench/bench/kernel/*
do
  if [ -d "$algorithm" ]
  then
    if [ -f "$algorithm/`basename $algorithm`.elf" ]
    then
        elf="$algorithm/`basename $algorithm`.elf"
        echo "-------------executing $elf------------------\n"
        ./bin/cacheAnalysis $elf main -c mycacheLRU.xml 

        echo "------------------Done------------------\n"
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
#        ./bin/cacheAnalysis $elf main -c mycacheLRU.xml 
#    else
#        echo "N/A"
#    fi
#  fi
#done
