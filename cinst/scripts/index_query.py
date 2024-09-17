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
print("IS_INDEXOF, OVERHEAD, OVERHEAD_SAVE, CLASSNAME, LINE")
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
    print(cop)
    return cop
contains_op = get_cop("contains(Ljava/lang/Object;)Z")
indexof_op = get_cop("indexOf(Ljava/lang/Object;)I")
lastindexof_op = get_cop("lastIndexOf(Ljava/lang/Object;)I")
get_op = get_cop("get(I)Ljava/lang/Object;")
'''
(SELECT HOLDER_ID FROM CONTAINERS GROUP BY HOLDER_ID HAVING SUM(CASE WHEN c1.copid NOT IN ({cops}) THEN 1 ELSE 0 END) = 0)
'''

cursor.execute(f'''--sql
SELECT
    CASE WHEN COPID = {indexof_op} THEN 1 ELSE 0 END AS IS_INDEXOF,
    SUM(
        CASE WHEN 
            COPID = {indexof_op} 
        THEN 
            (CASE WHEN V1 == 4294967295 THEN SIZE ELSE V1 END)
        ELSE
            (CASE WHEN V1 == 4294967295 THEN SIZE ELSE SIZE-V1 END)
        END
    )AS OVERHEAD,
    SUM(
        CASE WHEN 
            COPID = {indexof_op} 
        THEN
            (CASE WHEN V1 == 4294967295 THEN SIZE ELSE V1 END) -
            (CASE WHEN V1 == 4294967295 THEN SIZE ELSE SIZE-V1 END)
        ELSE
            (CASE WHEN V1 == 4294967295 THEN SIZE ELSE SIZE-V1 END) - 
            (CASE WHEN V1 == 4294967295 THEN SIZE ELSE V1 END)
        END
    ) AS OVERHEAD_SAVE,
    CLASSNAME.CLASSNAME,
    LINE
FROM CONTAINERS
INNER JOIN 
    CLASSNAME ON CLASS_NAME_ID + 1 = CLASSNAME.ID
WHERE copid in ( {indexof_op}, {lastindexof_op})
GROUP BY
    CLASS_NAME_ID, LINE
ORDER BY OVERHEAD_SAVE DESC
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
res = cursor.fetchmany(20)
for l in res:
    print(l)
