import os
from matplotlib import pyplot as plt
import re
import math
import glob
import numpy as np
import sys
os.chdir(sys.argv[1])
logs = glob.glob("*-*-*.log")
def std_err(x):
    return np.std(x)/(math.sqrt(len(x)))
def avg(x):
    return sum(x)/len(x)
def get_mem(log_file_name):
    with open(log_file_name) as f:
        lines = f.readlines()
    lines = [x for x in lines if "Maximum resident set size" in x]
    # print(log_file_name)
    kbs = [int(x.split(':')[-1]) for x in lines]
    return kbs[0]
def parse_log_name(log_file_name):
    res = re.match(r"(\w+)-(.+)-([^-]+)\.log", log_file_name)
    return res.groups()

records = {}
for log in logs:
    mode, case, marker = parse_log_name(log)
    info = get_mem(log)
    print(f'{mode}, {case}, {info}, {marker}')
    records.setdefault(case, {})
    records[case].setdefault(mode, [])
    records[case][mode].append(info)
exit()

for case in records:
    err = std_err(records[case]['java'])
    records[case]['java'] = avg(records[case]['java'])
    print(f"{case}: {err/records[case]['java']} {records[case]['java']/1024}")
    records[case]['s1'] = [x/records[case]['java'] for x in records[case]['ort']]
    records[case]['s2'] = [x/records[case]['java'] for x in records[case]['ort_use']]

s1_avgs = [avg(records[case]['s1']) for case in records]
s2_avgs = [avg(records[case]['s2']) for case in records]
s1_errs = [2*std_err(records[case]['s1']) for case in records]
s2_errs = [2*std_err(records[case]['s2']) for case in records]
labels = [case for case in records]


l = len(labels)
total_width, n = 0.8, 2
width = total_width/n
x = np.arange(l) - (total_width-width)/2
# print(x1)
plt.bar(x, s1_avgs, width=width, label="ort", fc='b')
plt.bar(x+width, s2_avgs, width=width, label="ort-use", fc='r')
# print("2")
plt.errorbar(x, s1_avgs, yerr=s1_errs, fmt="none", ecolor='black', capsize=5)
plt.errorbar(x+width, s2_avgs, yerr=s2_errs, fmt="none", ecolor='black', capsize=5)
print(avg(s1_avgs+s2_avgs))
plt.xticks(np.arange(l), labels, rotation=90)
plt.legend()
plt.tight_layout()
plt.savefig('mem.png')

