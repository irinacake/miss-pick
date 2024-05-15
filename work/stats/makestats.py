import os, json
from pathlib import Path
import pandas as pd
import statistics
import sys
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import numpy as np


new_data = {'bench': [], 'task': [], 'policy': [], 'asc': [], 'set_count': [], 'exec_time': [], 'exit': [], 'bb_count': [], 'total_bb': [], 'states_avg': [], 'max_states': [], 'states_total': []}


resultdir = os.path.join("results", sys.argv[1].split('/')[1])

Path(resultdir).mkdir(parents=True, exist_ok=True)

print(resultdir)


for jsonfile in sys.argv[1:]:
    try:
        exp = json.load(open(jsonfile, 'r'))
        new_data['bench'].append(os.path.basename(exp['file']))
        new_data['task'].append(exp['task'])
        new_data['policy'].append(exp['policy'])
        new_data['asc'].append(exp['associativity'])
        new_data['set_count'].append(exp['set_count'])
        new_data['exec_time'].append(exp['exec_time']/1000000)
        new_data['exit'].append(0)#.append(exp['exit_value'])
        new_data['bb_count'].append(exp['bb_count'][0])
        new_data['total_bb'].append(0)#.append(exp['total_bb'])
        new_data['states_avg'].append(statistics.fmean(exp['state_moys']))
        new_data['max_states'].append(max(exp['state_maxs']))
        new_data['states_total'].append(round(sum(exp['state_total'])))
    
    except:
        print("[WARNING] json file not valid : ", jsonfile)
        pass



resultats = pd.DataFrame(data=new_data)
pd.set_option('display.max_colwidth', None)
pd.set_option('display.max_rows', None)


print(resultats)#.sort_values('exec_time'))

resultats = resultats.reset_index()  # make sure indexes pair with number of rows

blue_triangle = mlines.Line2D([], [], color='b', marker='^', linestyle='None',
                          markersize=5, label='4x32')

green_star = mlines.Line2D([], [], color='g', marker='*', linestyle='None',
                          markersize=5, label='4x256')

red_plus = mlines.Line2D([], [], color='r', marker='+', linestyle='None',
                          markersize=5, label='8x32')

magenta_cross = mlines.Line2D([], [], color='m', marker='x', linestyle='None',
                          markersize=5, label='8x256')



#graph 1:
# states total for FIFO with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "FIFO":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_total'],'b^')
            else:
                plt.plot(row['bb_count'],row['states_total'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_total'],'r+')
            else:
                plt.plot(row['bb_count'],row['states_total'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Total States Count / BB Count for FIFO for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Total States Count")
plt.savefig(resultdir + "/TotalStates_FIFO")

#graph 2:
# states total for LRU with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "LRU":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_total'],'b^')
            else:
                plt.plot(row['bb_count'],row['states_total'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_total'],'r+')
            else:
                plt.plot(row['bb_count'],row['states_total'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Total States Count / BB Count for LRU for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Total States Count")
plt.savefig(resultdir + "/TotalStates_LRU")



#graph 3:
# states total for PLRU with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "PLRU":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_total'],'b^')
            else:
                plt.plot(row['bb_count'],row['states_total'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_total'],'r+')
            else:
                plt.plot(row['bb_count'],row['states_total'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Total States Count / BB Count for PLRU for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Total States Count")
plt.savefig(resultdir + "/TotalStates_PLRU")




#graph 4:
# states input moy for FIFO with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "FIFO":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_avg'],'b^')
            else:
                plt.plot(row['bb_count'],row['states_avg'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_avg'],'r+')
            else:
                plt.plot(row['bb_count'],row['states_avg'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Moy States Count for a BB / BB Count for FIFO for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Moy States Count")
plt.savefig(resultdir + "/StatesMoy_FIFO")




#graph 5:
# states input moy for LRU with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "LRU":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_avg'],'b^')
            else:
                plt.plot(row['bb_count'],row['states_avg'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_avg'],'r+')
            else:
                plt.plot(row['bb_count'],row['states_avg'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Moy States Count for a BB / BB Count for LRU for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Moy States Count")
plt.savefig(resultdir + "/StatesMoy_LRU")




#graph 6:
# states input moy for PLRU with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "PLRU":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_avg'],'b^')
            else:
                plt.plot(row['bb_count'],row['states_avg'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['states_avg'],'r+')
            else:
                plt.plot(row['bb_count'],row['states_avg'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Moy States Count for a BB / BB Count for PLRU for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Moy States Count")
plt.savefig(resultdir + "/StatesMoy_PLRU")





#graph 7:
# states input max for FIFO with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "FIFO":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['max_states'],'b^')
            else:
                plt.plot(row['bb_count'],row['max_states'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['max_states'],'r+')
            else:
                plt.plot(row['bb_count'],row['max_states'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Max States Count for a BB / BB Count for FIFO for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Max States Count")
plt.savefig(resultdir + "/StatesMax_FIFO")




#graph 8:
# states input max for LRU with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "LRU":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['max_states'],'b^')
            else:
                plt.plot(row['bb_count'],row['max_states'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['max_states'],'r+')
            else:
                plt.plot(row['bb_count'],row['max_states'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Max States Count for a BB / BB Count for LRU for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Max States Count")
plt.savefig(resultdir + "/StatesMax_LRU")




#graph 9:
# states input max for PLRU with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "PLRU":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['max_states'],'b^')
            else:
                plt.plot(row['bb_count'],row['max_states'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['max_states'],'r+')
            else:
                plt.plot(row['bb_count'],row['max_states'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Max States Count for a BB / BB Count for PLRU for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Max States Count")
plt.savefig(resultdir + "/StatesMax_PLRU")



#graph 10:
# exec time for FIFO with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "FIFO":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['exec_time'],'b^')
            else:
                plt.plot(row['bb_count'],row['exec_time'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['exec_time'],'r+')
            else:
                plt.plot(row['bb_count'],row['exec_time'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Exec time / BB Count for FIFO for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Exec Time (s)")
plt.savefig(resultdir + "/ExecTime_FIFO")



#graph 11:
# exec time for LRU with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "LRU":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['exec_time'],'b^')
            else:
                plt.plot(row['bb_count'],row['exec_time'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['exec_time'],'r+')
            else:
                plt.plot(row['bb_count'],row['exec_time'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Exec time / BB Count for LRU for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Exec Time (s)")
plt.savefig(resultdir + "/ExecTime_LRU")





#graph 12:
# exec time for PLRU with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "PLRU":
        if row['asc'] == 4:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['exec_time'],'b^')
            else:
                plt.plot(row['bb_count'],row['exec_time'],'g*')
        else:
            if row['set_count'] == 32:
                plt.plot(row['bb_count'],row['exec_time'],'r+')
            else:
                plt.plot(row['bb_count'],row['exec_time'],'mx')
plt.legend(handles=[blue_triangle, green_star, red_plus, magenta_cross], loc='upper left')
plt.title("Exec time / BB Count for PLRU for every configuration")
plt.xlabel("BB Count")
plt.ylabel("Exec Time (s)")
plt.savefig(resultdir + "/ExecTime_PLRU")