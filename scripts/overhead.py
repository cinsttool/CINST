from matplotlib import pyplot as plt
import numpy as np
import math
import sys
import scipy.stats as stats
if len(sys.argv) == 2 and sys.argv[1] == '2':
    files = ['mem', 'mem']
    ylabel = 'Memory Bloat'
else:
    files = ['real', 'time']
    ylabel = 'Runtime Slowdown'
# files = ['real', 'time']

def confidence_interval(x):
    geometric_mean = stats.gmean(x)
    
    log_values = np.log(x)
    std_dev = np.std(log_values)

    # Step 3: Calculate the 95% confidence interval for the geometric mean
    n = len(x)  # Number of observations (30 in this case)
    z_score = stats.norm.ppf(0.975)  # Z-score for 95% confidence
    margin_of_error = z_score * (std_dev / np.sqrt(n))
    return (geometric_mean - np.exp(np.log(geometric_mean) - margin_of_error), 
                       np.exp(np.log(geometric_mean) + margin_of_error) - geometric_mean)




def list_map(self:list, func):
    return [func(x) for x in self]
def list_lreduce(self:list, func):
    self = list(self)
    if len(self) == 0:
        return []
    
    if len(self) == 1:
        return self
    res = func(self[0], self[1])
    for i in range(2, len(self)):
        res = func(res, self[i])
    return res

        

def median(x):
    x = sorted(x)
    if len(x) % 2 == 0:
        return (x[len(x)//2-1]+x[len(x)//2])/2
    else:
        return x[len(x)//2]

def avg(x):
    return sum(x)/max(len(x), 1)
def geoavg(x):
    res = 1
    for y in x:
        res *= y
    return res ** (1/len(x))
    
def std_err(x):
    return np.std(x)/(math.sqrt(len(x)))
def get_avg_and_err(d):
    ks = list(d.keys())
    avgs = []
    errs = []
    for k in ks:
        avgs.append(avg([x for x in d[k] if x != 0]))
        errs.append(std_err([x for x in d[k] if x != 0])*2)
    return ks, avgs, errs


with open('dacapo.txt') as f:
    dacapo = f.readlines()
    dacapo = [x.strip() for x in dacapo]

with open('renaissance.txt') as f:
    renaissance = f.readlines()
    renaissance = [x.strip() for x in renaissance]
with open('spec.txt') as f:
    spec = f.readlines()
    spec = [x.strip() for x in spec]


with open(f"{files[0]}.log") as f:
    data = f.readlines()
data = [x.split(',') for x in data]
data = [[y.strip() for y in x] for x in data]
# data = [(x[0], float(x[2])/float(x[1]), float(x[3])/float(x[1])) for x in data]
data_by_marker = {}
for line in data:
    data_by_marker.setdefault(line[-1], {'name':line[1]})
    if line[2] != '':
        data_by_marker[line[-1]][line[0]] = float(line[2])
for k in data_by_marker: 
    data_by_marker[k]['s1'] = data_by_marker[k].get('ort', 0)/data_by_marker[k]['java']
    data_by_marker[k]['s2'] = data_by_marker[k].get('ort_use', 0)/data_by_marker[k]['java']
    # if data_d[k]['s1'] > 30 or data_d[k]['s2'] > 30 or data_d[k]['s1'] == 0 or data_d[k]['s2'] == 0:
    #     data_d[k]['s1'] = 0
    #     data_d[k]['s2'] = 0
# print(data_by_marker)

data_by_name = {}
for marker in data_by_marker:
    item = data_by_marker[marker]
    data_by_name.setdefault(item['name'], {})
    for k in ['java', 'ort_use', 'ort']:
        data_by_name[item['name']].setdefault(k, [])
        data_by_name[item['name']][k].append(item[k])



for name in data_by_name:
    item = data_by_name[name]
    item['avg_java'] = avg(item['java'])
    item['s1'] = [ort / item['avg_java'] for ort in item['ort']]
    item['s2'] = [ort_use / item['avg_java'] for ort_use in item['ort_use']]
    item['err1'] = confidence_interval(item['s1'])
    item['err2'] = confidence_interval(item['s2'])
# print(data_by_name)

data_list = []
for name in data_by_name:
    item = data_by_name[name]
    item['name'] = name
    data_list.append(item)



def group_by(data_list, groups):
    res = {}
    for item in data_list:
        for group_name in groups:
            if item['name'] in groups[group_name]:
                res.setdefault(group_name, [])
                res[group_name].append(item)
                break
    for group_name in res:
        res[group_name].sort(key=lambda x: max(x['s1'], x['s2']))
    return res

data_group = group_by(data_list, {
    'Dacapo 9.2': dacapo,
    'Renaissance': renaissance,
    "SPECjvm2008": spec
})

labels = []
x1_avgs = []
x1_errs = []
x2_avgs = []
x2_errs = []
names = []
group_size_list = []

for group_name in sorted(data_group, reverse=False):
    names.append(group_name)
    group_size_list.append(len(data_group[group_name]))
    if len(group_size_list) > 1:
        group_size_list[-1] += group_size_list[-2]
    for item in data_group[group_name]:
        labels.append(item['name'])
        x1_avgs.append(avg(item['s1']))
        x2_avgs.append(avg(item['s2']))
        x1_errs.append(item['err1'])
        x2_errs.append(item['err2'])


geo_x1 = geoavg(x1_avgs)
median_x1 = median(x1_avgs)
geo_x2 = geoavg(x2_avgs)
median_x2 = median(x2_avgs)

print(f'geo x1: {geo_x1}, geo x2: {geo_x2}')
print(f'median x1: {median_x1}, median x2: {median_x2}')

def get_percent(x, p):
    x = sorted(x)
    return x[int(len(x)*p)]
p = 0.9
hline_y1 = get_percent(x1_avgs,p)
hline_y2 = get_percent(x2_avgs,p)
print(f'ref percent {p}: {hline_y1}')
print(f'use percent {p}: {hline_y2}')

x1_avgs += [geo_x1, median_x1]
x2_avgs += [geo_x2, median_x2]

labels.append("GeoMean")
labels.append("Median")



print("0")
# x1s = {}
# [x1s.setdefault(data_by_marker[x]['name'], []).append((data_by_marker[x]['s1'])) for x in data_by_marker]
# x2s = {}
# [x2s.setdefault(data_by_marker[x]['name'], []).append((data_by_marker[x]['s2'])) for x in data_by_marker]
# 
# x1 = [x[1] for x in data]
# x2 = [x[2] for x in data]
# 
# labels, x1_avgs, x1_errs = get_avg_and_err(x1s)
# labels, x2_avgs, x2_errs = get_avg_and_err(x2s)
# print(x2s)
# print("1")

def filter_label(labels):
    res = []
    for label in labels:
        if len(label.split('.')) > 2:
            label = ".".join(label.split('.')[:-1])
        if len(label) > 15:
            label = label.split('-')[-1]
        if len(label) > 15:
            label = label.split('.')[-1]
        res.append(label)
    return res

labels = filter_label(labels)
l = len(labels)
total_width, n = 0.8, 2
width = total_width/n
x = np.arange(l) - (total_width-width)/2
fig = plt.figure(figsize=(10,3.5))
print("1")
ax = fig.add_subplot(111)
# print(x1)
plt.rcParams["font.size"] = 8
ax.bar(x, x1_avgs, width=width, label="ref", fc='#3c80bb')
ax.bar(x+width, x2_avgs, width=width, label="use", fc='#a8d9b3')
print("2")
x1_errs = np.array(x1_errs).transpose()
x2_errs = np.array(x2_errs).transpose()
plt.errorbar(x[:-2], x1_avgs[:-2], yerr=x1_errs, fmt="none", ecolor='black', capsize=2)
plt.errorbar((x+width)[:-2], x2_avgs[:-2], yerr=x2_errs, fmt="none", ecolor='black', capsize=2)
plt.axhline(hline_y1, 0, 1000, color='black', linestyle='dashed', linewidth=0.5)
plt.axhline(hline_y2, 0, 1000, color='black', linestyle='dashed', linewidth=0.5)

# plt.text(l, hline_y1+0.1, f'{hline_y1:.1f}')
# plt.text(l, hline_y2+0.1, f'{hline_y2:.1f}')
for i in range(len(names)):
    plt.axvline(group_size_list[i]-1+0.5, 0, -0.5, color = "black", linestyle="dashed", clip_on = False, linewidth=0.5)
    left = 0
    if i > 0:
        left = group_size_list[i-1]
    right = group_size_list[i]
    plt.text(x=(left+right)/2, y=-max(x1_avgs+x2_avgs)/2*1.1, s=names[i], color='black', ha='center', va='center', clip_on=False)
# plt.axvline(10.5, 0, -0.5, color = "black", linestyle="dashed", clip_on = False, linewidth=0.5)
# plt.axvline(len(labels)-3+0.5, 0, -0.5, color = "black", linestyle="dashed", clip_on = False, linewidth=0.5)
# plt.text(x=7*width*2, y = -max(x1_avgs+ x2_avgs)/2, s='Dacapo 9.2', color='black', ha='center', va='center', clip_on=False)
# plt.text(x=21*width*2, y = -max(x1_avgs+ x2_avgs)/2, s='Renaissance', color='black', ha='center', va='center', clip_on=False)
plt.xticks(np.arange(l), labels, rotation=90, fontsize=8)
ax.set_ylabel(ylabel, {'fontsize': 8})
print("3")
# ax.set_xticks(np.arange(l), labels, rotation=90)
ax2 = ax.twinx()
hs = [hline_y1, hline_y2]
ax2.set_yticks(hs, [f'{x:.1f}' for x in hs])
ax2.set_ylim(ax.get_ylim())
# plt.text()
ax.legend()
plt.tight_layout()
fig.savefig(f'{files[1]}.pdf', bbox_inches='tight')
fig.savefig(f'{files[1]}.png', bbox_inches='tight')
print(labels)
