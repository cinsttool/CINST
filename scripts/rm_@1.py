import sys

timelog = sys.argv[1]

with open(timelog, 'r') as f:
    data = f.readlines()
    
data = [x.strip() for x in data]

dataMap = {}

for d in data:
    l = d.split(", ")
    # print(l)
    if dataMap.get(l[1]) == None:
        dataMap[l[1]] = {}
    if l[-1][-2] == "@":
        if dataMap[l[1]].get(l[-1][:-6]) == None:
            dataMap[l[1]][l[-1][:-6]] = {}
        dataMap[l[1]][l[-1][:-6]][l[0]] = l[2]
    else:
        if dataMap[l[1]].get(l[-1]) == None:
            dataMap[l[1]][l[-1]] = {}
        dataMap[l[1]][l[-1]][l[0]] = l[2]
        
for c in dataMap:
    for t in dataMap[c]:
        print(f"java, {c}, {dataMap[c][t]['java']}, {t}")
        print(f"ort, {c}, {dataMap[c][t]['ort']}, {t}")
        # print(f"{c}, {dataMap[c][t]['java']}, {dataMap[c][t]['ort']}, {dataMap[c][t]['ort_use']}, {t}")