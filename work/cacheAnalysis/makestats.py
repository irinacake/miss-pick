import os, json
from pathlib import Path
import pandas as pd
import statistics



jsonresults = json.load(open("results/results2.json",'r'))

exps = jsonresults['exp']

new_data = {'bench': [], 'task': [], 'policy': [], 'associativity': [], 'set_count': [], 'exec_time': [], 'bb_count': [], 'states_avg': [], 'max_states_avg': [], 'states_total': []}

i = 0
for exp in exps:

    new_data['bench'].append(os.path.basename(exp['file']))
    new_data['task'].append(exp['task'])
    new_data['policy'].append(exp['policy'])
    new_data['associativity'].append(exp['associativity'])
    new_data['set_count'].append(exp['set_count'])
    new_data['exec_time'].append(exp['exec_time'])
    new_data['bb_count'].append(exp['bb_count'][0])
    new_data['states_avg'].append(statistics.fmean(exp['state_moys']))
    new_data['max_states_avg'].append(statistics.fmean(exp['state_maxs']))
    new_data['states_total'].append(round(sum(exp['state_moys']*exp['bb_count'][0])))



resultats = pd.DataFrame(data=new_data)
pd.set_option('display.max_colwidth', None)
pd.set_option('display.max_rows', None)

print(resultats)