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
'''
SELECT
    HOLDER_ID,
    MAX(SIZE) AS MSIZE
FROM 
    CONTAINERS
GROUP BY
    HOLDER_ID
'''
print('WASTE_RATIO, WASTED_SPACE, SMALL_CONTAINER_COUNT, CLASSNAME, LINE')
cursor.execute(f'''
SELECT
    CAST(SUM(MSIZE) AS FLOAT)/SUM(INIT_SIZE) AS RATIO,
    SUM(INIT_SIZE)-SUM(MSIZE) AS WASTED_SPACE,
    COUNT(1) AS SMALL_COUNT,
    CLASSNAME,
    ALLOCATIONS.LINE
FROM
    ALLOCATIONS
LEFT JOIN
    (SELECT
        HOLDER_ID,
        MAX(SIZE) AS MSIZE
    FROM
        CONTAINERS
    GROUP BY
        HOLDER_ID) C ON allocations.id = C.holder_id
JOIN
    CLASSNAME ON ALLOCATIONS.CLASS_NAME_ID+1 = CLASSNAME.ID
WHERE
    MSIZE < INIT_SIZE
GROUP BY
    ALLOCATIONS.CLASS_NAME_ID, ALLOCATIONS.LINE
ORDER BY WASTED_SPACE DESC
               ''')
res = cursor.fetchmany(20)
for l in res:
    print(l)
