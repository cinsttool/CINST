#!/usr/bin/env python3
import sqlite3
import os
import argparse
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
print('NULL_COUNT, ALL_COUNT, NULL_RATIO, CLASSNAME, LINE')
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
cops = cursor.execute(f'SELECT COP FROM CONTAINER_OP WHERE COP_NAME LIKE "add%"').fetchall()
print(cops)
cops = [str(x[0]) for x in cops]
cops = ','.join(cops)
'''
(SELECT HOLDER_ID FROM CONTAINERS GROUP BY HOLDER_ID HAVING SUM(CASE WHEN c1.copid NOT IN ({cops}) THEN 1 ELSE 0 END) = 0)
'''
cursor.execute(f'''---sql
SELECT 
    SUM(CASE WHEN (REF_ID == 0) THEN 1 ELSE 0 END) AS NULL_COUNT,
    COUNT(1) AS ALL_COUNT,
    SUM(CASE WHEN REF_ID == 0 THEN 1 ELSE 0 END)/CAST(COUNT(1) AS DOUBLE) AS NULL_RATIO,
    CLASSNAME,
    ALLOCATIONS.LINE
FROM 
    allocations
INNER JOIN
    (SELECT
        HOLDER_ID,
        MAX(SIZE) AS MSIZE
    FROM
        CONTAINERS
    GROUP BY
        HOLDER_ID
    HAVING
        MSIZE > 0) C ON allocations.id = C.holder_id
LEFT JOIN 
    containers c1 ON allocations.id = c1.holder_id
JOIN
    CLASSNAME ON ALLOCATIONS.CLASS_NAME_ID+1 = CLASSNAME.ID
WHERE
    copid IN ({cops})
GROUP BY 
    ALLOCATIONS.CLASS_NAME_ID, ALLOCATIONS.LINE
ORDER BY ALL_COUNT*NULL_RATIO DESC;

               ''')
res = cursor.fetchmany(20)
for l in res:
    print(l)
