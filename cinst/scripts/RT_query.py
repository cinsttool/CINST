#!/usr/bin/env python3
import sqlite3
import os
import argparse
'''
container misuse 
ArrayList -> Hash-base container
query overhead of contains/indexOf/lastIndexOf
contains will call indexOf
'''
#pid = 58371

# parser = argparse.ArgumentParser()
# parser.add_argument('pid', type=int, help='pid of jvm')
# args = parser.parse_args()
# pid = args.pid
pid = os.path.abspath('.').split('-')[-1]
int(pid)

db_name = f'{pid}.db'
conn = sqlite3.connect(db_name)
cursor = conn.cursor()
print("PREV_LOOP_GET_COUNT, NEXT_LOOP_GET_COUNT, LOOP_COUNT, PREV_LOOP_POS, NEXT_LOOP_POS")
# cursor.execute(f'''---sql
# SELECT 
#     CLASSNAME,
#     ALLOCATIONS.LINE,
#     COUNT(DISTINCT CASE WHEN copid IN (0, 1, 2, 3,4,5,6) THEN holder_id END) AS count_copid_1_3,
#     COUNT(DISTINCT holder_id) AS total_containers,
#     COUNT(DISTINCT CASE WHEN copid IN (0,1,2, 3,4,5,6) THEN holder_id END) / COUNT(DISTINCT holder_id) AS ratio
# FROM 
#     allocations
# JOIN 
#     containers ON allocations.id = containers.holder_id
# JOIN
#     CLASSNAME ON CLASSNAME.ID = ALLOCATIONS.ID
# GROUP BY 
#     ALLOCATIONS.CLASS_NAME_ID, ALLOCATIONS.LINE;

#                ''')
def get_cop(cop_name):
    cop = cursor.execute(f'SELECT COP FROM CONTAINER_OP WHERE COP_NAME = "{cop_name}"').fetchone()
    if cop is None:
        return -2
    cop = cop[0]
    # print(cop)
    return cop
contains_op = get_cop("contains(Ljava/lang/Object;)Z")
indexof_op = get_cop("indexOf(Ljava/lang/Object;)I")
lastindexof_op = get_cop("lastIndexOf(Ljava/lang/Object;)I")
get_op = get_cop("get(I)Ljava/lang/Object;")
add_op = get_cop("add(Ljava/lang/Object;)Z")
add2_op = get_cop("add(ILjava/lang/Object;)V")
addall_op = get_cop("addAll(Ljava/util/Collection;)Z")
remove_op = get_cop("remove(I)Ljava/lang/Object;")
'''
(SELECT HOLDER_ID FROM CONTAINERS GROUP BY HOLDER_ID HAVING SUM(CASE WHEN c1.copid NOT IN ({cops}) THEN 1 ELSE 0 END) = 0)
'''

cursor.execute(f'''--sql
SELECT
    holder_id, cid, copid, size, v1, v2, classname.classname, line
FROM
    containers
INNER JOIN
    classname on classname.id = containers.class_name_id + 1
WHERE holder_id != 4294967295 AND copid in ({get_op}, {add_op}, {add2_op}, {addall_op}, {remove_op})
ORDER BY TIME;
               ''')
# cursor.execute(f'''---sql
# SELECT 
#     SUM(
#         CASE 
#             WHEN c1.copid == {indexof_op} THEN (CASE WHEN V1 == 4294967295 THEN SIZE ELSE V1 END)
#             WHEN c1.copid == {lastindexof_op} THEN (CASE WHEN V1 == 4294967295 THEN SIZE ELSE SIZE - V1 END)
#             ELSE 0 END
#         ) AS OVERHEAD,
#     CAST(SUM(
#         CASE 
#             WHEN c1.copid == {indexof_op} THEN (CASE WHEN V1 == 4294967295 THEN SIZE ELSE V1 END)
#             WHEN c1.copid == {lastindexof_op} THEN (CASE WHEN V1 == 4294967295 THEN SIZE ELSE SIZE - V1 END)
#             ELSE 0 END
#         ) AS FLOAT) / SUM (CASE WHEN c1.copid in ({indexof_op}, {lastindexof_op}) THEN 1 ELSE 0 END) AS AVG_OVERHEAD ,
#     SUM(CASE WHEN c1.copid == {get_op} THEN 1 ELSE 0 END),
#     MAX(CASE WHEN c1.copid == {indexof_op} THEN SIZE ELSE 0 END),
    
#     SUM(CASE WHEN c1.copid == {indexof_op} THEN 1 ELSE 0 END),
#     SUM(CASE WHEN c1.copid == {lastindexof_op} THEN 1 ELSE 0 END),
#     CLASSNAME,
#     ALLOCATIONS.LINE
# FROM 
#     allocations
# INNER JOIN
#     (SELECT
#         ID,
#         TYPE
#     FROM 
#         TYPEMAP
#     WHERE
#         TYPE = "java/util/ArrayList"
#         OR TYPE = "java/util/LinkedList"
#     ) TM ON TM.ID = ALLOCATIONS.TYPEID 
# INNER JOIN 
#     containers c1 ON allocations.id = c1.holder_id
# JOIN
#     CLASSNAME ON ALLOCATIONS.CLASS_NAME_ID+1 = CLASSNAME.ID
# WHERE copid in ( {indexof_op}, {lastindexof_op}, {get_op})
# GROUP BY 
#     ALLOCATIONS.CLASS_NAME_ID, ALLOCATIONS.LINE
# ORDER BY OVERHEAD*AVG_OVERHEAD DESC;

#                ''')
def is_const(op):
    return op == get_op
res = cursor.fetchall()
LOOP_GET_OP = -2

class ContainerOp:
    def __init__(self, hid, cid, copid, size, v1, v2, classname, line, count=1):
        self.hid = hid
        self.cid = cid
        self.copid = copid
        self.size = size
        self.v1 = v1
        self.v2 = v2
        self.classname = classname
        self.line = line
        self.count = count
    
    def is_const(self):
        return self.is_op(LOOP_GET_OP) or self.is_op(get_op)
    def is_op(self, op_id):
        return self.copid == op_id
    def is_prev_get(self, op: "ContainerOp"):
        assert(self.is_op(LOOP_GET_OP) or self.is_op(get_op))
        return self.v1 + 1 == op.v1 and op.is_op(get_op) and self.classname == op.classname and self.line == op.line
    def add_op_for_loop(self, op: "ContainerOp"):
        assert(self.copid == LOOP_GET_OP or self.copid == get_op)
        assert(op.copid == get_op)
        assert(op.classname == self.classname)
        assert(op.line == self.line)
        assert(self.v1 + 1 == op.v1)
        if self.copid == LOOP_GET_OP:
            self.count += op.count
            self.v1 += 1
            return self
        else:
            res = ContainerOp.create_loop_op(self)
            return res.add_op_for_loop(op)
    def pos(self):
        return f'{self.classname}:{self.line}'
    
    @staticmethod
    def create_loop_op(op: "ContainerOp"):
        assert(op.copid == get_op or op.copid == LOOP_GET_OP)
        loop_op = ContainerOp(op.hid, op.cid, LOOP_GET_OP, op.size, op.v1, op.v2, op.classname, op.line, op.count)
        return loop_op
    
    def __repr__(self) -> str:
        return f'({self.hid} {self.copid} {self.count} {self.pos()})'


groups = {}
for l in res:
    groups.setdefault(l[0], [])
    groups[l[0]].append(l)

# print(groups)
out = {}
for hid, group in groups.items():
    last = None
    out[hid] =  []
    for l in group:
        op = ContainerOp(*l)
        if last is not None and last.is_const() and op.copid == get_op:
            if (last.is_prev_get(op)):
                last = last.add_op_for_loop(op)
                continue
            else:
                out[hid].append(last)
        elif last is not None and not last.is_const() and op.copid == get_op:
            out[hid].append(last)
        elif last is not None and last.is_const() and op.copid != get_op:
            out[hid].append(last)
        last = op
    out[hid].append(last)
    # out[hid] = [x for x in out[hid] if x.count > 5 or x.is_const()]

groups = out
# print(groups)

res = {}

class TraverseInfo:
    def __init__(self, pos):
        self.prev_count = 0
        self.next_count = 0
        self.loop_count = 0
        self.pos = pos
for hid, group in groups.items():
    last = None
    for op in group:
        if last is None:
            last = op
            continue
        if last.is_const() and op.is_const() and last.count > 5 and op.count > 5:
            key = (last.pos(), op.pos())
            res.setdefault(key, TraverseInfo(key))
            res[key].prev_count += last.count
            res[key].next_count += op.count
            res[key].loop_count += 1
        last = op

# print(groups)
res = list(res.values())
res.sort(key=lambda x: min(x.prev_count, x.next_count), reverse=True)

for i, info in enumerate(res):
    if i > 20:
        break
    print(f'{info.prev_count} {info.next_count} {info.loop_count} {info.pos}')



# for l in res:
#     if l[2] == get_op:
#         if last_get is not None and last_get[-1] == l[-1] and last_get[-2] == l[-2] and last_get[0] == l[0]:
#             last_get_counter += 1
#             last_get = l
#         elif last_get is not None:
#             groups.append((last_get, last_get_counter))
#             last_get = l
#             last_get_counter = 1
#         else:
#             last_get = l
#             last_get_counter = 1
#     elif last_get is not None:
#         groups.append((last_get, last_get_counter))
#         last_get = None
#         last_get_counter = 0
#         groups.append(l)
#     else:
#         last_get = None
#         last_get_counter = 0
#         groups.append(l)
# def is_same(a, b):
#     if len(a) != len(b):
#         return False
#     if len(a) != 2:
#         return False
#     return a[0][0] == b[0][0] and a[0][-1] == b[0][-1] and a[0][-2] == b[0][-2]
# modified = False
# last_get = None
# print(groups)
# for i in range(0, len(groups)):
#     if len(groups[i]) == 2:
#         item = groups[i]
#         if last_get is not None and not modified and is_same(last_get, item):
#             print(last_get, item)
#         last_get = item
#         modified = False
#     elif not is_const(groups[i][2]):
#         modified = True
