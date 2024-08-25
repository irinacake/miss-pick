import os, json
from pathlib import Path
import pandas as pd
import statistics
import sys
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import numpy as np


#new_data = {'bench': [], 'task': [], 'policy': [], 'asc': [], 'setc': [], 'exec_t(ms)': [], 'exit': [], 'bb_count': [], 'bb_total': [], 'states_avg': [], 'max_states': [], 'states_total': []}
new_data = {'task': [], 'policy': [], 'asc': [], 'setc': [], 'exec_t(ms)': [], 'exit': [], 'bb_cnt_avg': [], 'used_avg': [], 'bb_total': [], 's_avg': [], 's_max': [], 's_total': [], "ahcpt" : [], "amcpt" : [], "nccpt" : [], "fmcpt" : [], "WCET" : []}


resultdir = os.path.join("results", sys.argv[1].split('/')[1])

Path(resultdir).mkdir(parents=True, exist_ok=True)

print(resultdir)


for jsonfile in sys.argv[1:]:
    try:
        exp = json.load(open(jsonfile, 'r'))
        #new_data['bench'].append(os.path.basename(exp['file']))
        new_data['task'].append(exp['task'])
        new_data['policy'].append(exp['policy'])
        new_data['asc'].append(exp['associativity'])
        new_data['setc'].append(exp['set_count'])
        new_data['exec_t(ms)'].append(round(exp['exec_time']/1000,2))
        new_data['exit'].append(exp['exit_value'])
        new_data['bb_cnt_avg'].append(round(statistics.fmean(exp['bb_count']),2))
        new_data['used_avg'].append(round(statistics.fmean(exp['used_bb_count']),2))
        new_data['bb_total'].append(exp['total_bb'][0])
        new_data['s_avg'].append(round(statistics.fmean(exp['state_moys']),2))
        new_data['s_max'].append(max(exp['state_maxs']))
        new_data['s_total'].append(round(sum(exp['state_total'])))
        new_data['ahcpt'].append(exp['_ahcpt'])
        new_data['amcpt'].append(exp['_amcpt'])
        new_data['nccpt'].append(exp['_nccpt'])
        new_data['fmcpt'].append(exp['_fmcpt'])
        new_data['WCET'].append(exp['WCET'])
    
    except:
        print("[WARNING] json file not valid : ", jsonfile)
        pass



resultats = pd.DataFrame(data=new_data)
pd.set_option('display.max_colwidth', None)
pd.set_option('display.max_rows', None)


print(resultats)#.sort_values('exec_t(ms)'))

resultats = resultats.reset_index()  # make sure indexes pair with number of rows

blue_triangle = mlines.Line2D([], [], color='b', marker='^', linestyle='None',
                          markersize=5, label='FIFO')

green_star = mlines.Line2D([], [], color='g', marker='*', linestyle='None',
                          markersize=5, label='LRU')

red_plus = mlines.Line2D([], [], color='r', marker='+', linestyle='None',
                          markersize=5, label='PLRU')



#graph 1:
# states total based on policy
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "FIFO":
        plt.plot(row['bb_cnt_avg'],row['s_total'],'b^')
    if row['policy'] == "LRU":
        plt.plot(row['bb_cnt_avg'],row['s_total'],'g*')
    if row['policy'] == "PLRU":
        plt.plot(row['bb_cnt_avg'],row['s_total'],'r+')

plt.legend(handles=[blue_triangle, green_star, red_plus], loc='upper left')
plt.title("Total States Count / BB Count / Policy")
plt.xlabel("BB Count")
plt.ylabel("Total States Count")
plt.savefig(resultdir + "/TotalStates")




#graph 2:
# states input moy for FIFO with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "FIFO":
        plt.plot(row['bb_cnt_avg'],row['s_avg'],'b^')
    if row['policy'] == "LRU":
        plt.plot(row['bb_cnt_avg'],row['s_avg'],'g*')
    if row['policy'] == "PLRU":
        plt.plot(row['bb_cnt_avg'],row['s_avg'],'r+')
plt.legend(handles=[blue_triangle, green_star, red_plus], loc='upper left')
plt.title("Moy States Count / BB Count / Policy")
plt.xlabel("BB Count")
plt.ylabel("Moy States Count")
plt.savefig(resultdir + "/StatesMoy")



#graph 3:
# states input max for FIFO with all 4 configurations
plt.clf()
for index, row in resultats.iterrows():
    if row['policy'] == "FIFO":
        plt.plot(row['bb_cnt_avg'],row['s_max'],'b^')
    if row['policy'] == "LRU":
        plt.plot(row['bb_cnt_avg'],row['s_max'],'g*')
    if row['policy'] == "PLRU":
        plt.plot(row['bb_cnt_avg'],row['s_max'],'r+')
plt.legend(handles=[blue_triangle, green_star, red_plus], loc='upper left')
plt.title("Max States Count / BB Count / Policy")
plt.xlabel("BB Count")
plt.ylabel("Max States Count")
plt.savefig(resultdir + "/StatesMax")





#graph 4:
# exec time for FIFO with all 4 configurations

plt.clf()
for index, row in resultats.iterrows():
    if row['exec_t(ms)'] < 2700:
        if row['policy'] == "FIFO":
            plt.plot(row['bb_cnt_avg'],row['exec_t(ms)'],'b^')
        if row['policy'] == "LRU":
            plt.plot(row['bb_cnt_avg'],row['exec_t(ms)'],'g*')
        if row['policy'] == "PLRU":
            plt.plot(row['bb_cnt_avg'],row['exec_t(ms)'],'r+')
plt.legend(handles=[blue_triangle, green_star, red_plus], loc='upper left')
plt.title("Exec time / BB Count / Policy")
plt.xlabel("BB Count")
plt.ylabel("Exec Time (s)")
plt.savefig(resultdir + "/ExecTime")




'''



'''
