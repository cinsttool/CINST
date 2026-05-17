import sys

with open("renaissance.txt") as f:
    r_task = f.readlines()

r_task = [t.strip() for t in r_task]

files = ['real_cinst', 'mem']

with open(f"{files[0]}.log") as f:
    real_data = f.readlines()
real_data = [x.split(',') for x in real_data]
real_data = [[y.strip() for y in x] for x in real_data]

with open(f"{files[1]}.log") as f:
    mem_data = f.readlines()
mem_data = [x.split(',') for x in mem_data]
mem_data = [[y.strip() for y in x] for x in mem_data]

merge_data = [None]*int(len(mem_data)/2)*3
for i in range(0, len(mem_data), 2):
    merge_data[int(i/2*3)+0] = real_data[i]
    merge_data[int(i/2*3)+1] = real_data[i+1]
    align_mem_ort = None
    align_mem_java = None
    for md in mem_data:
        if md[3] == real_data[i][3]:
            if md[0] == 'java':
                align_mem_java = md
            else:
                align_mem_ort = md
        if align_mem_ort != None and align_mem_java != None:
            break
    merge_data[int(i/2*3)+2] = ['ort_use', real_data[i][1], str(float(align_mem_ort[2])/float(align_mem_java[2])*float(real_data[i][2])), align_mem_java[3]]


for i in range(0, len(merge_data)):
    print(f'{merge_data[i][0]}, {merge_data[i][1]}, {merge_data[i][2]}, {merge_data[i][3]}')
