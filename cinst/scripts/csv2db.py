#!/usr/bin/env python3
import sqlite3
import glob
import os
import argparse 
#pid = 46051
# parser = argparse.ArgumentParser()
# parser.add_argument('pid', type=int, help='pid of jvm')
# args = parser.parse_args()
# pid = args.pid
pid = os.path.abspath('.').split('-')[-1]
int(pid)
#pid = 58371
dbname = f'{pid}.db'

conn = sqlite3.connect(dbname)
cursor = conn.cursor()
'''
For PutField/AAStore, ADDR0 means holder and ADDR1 means ref;
for GC, ADDR0 means src and ADDR1 means dest;
for NEW, ADDR0 means object address.

For AAStore, ID means array index;
for NEw, ID means object type id;
for PutField, ID means field id.
| 64    | 64    | 64        | 32    | 32    | 32            | 8             | -             |
| TIME  | ADDR0 | ADDR1     | ID    | LINE  | CLASS_NAME_ID | TYPE          | TID           |
| time  | addr  | obj_size  | TYPEID| LINE  | CLASS_NAME_ID | n(new)        | primary key   |
| time  | holder| ref       | INDEX | LINE  | CLASS_NAME_ID | a(aastore)    | ~             |
| time  | holder| ref       | FIELD | LINE  | CLASS_NAME_ID | p(putfield)   | ~             |
| time  | src   | dest      | NULL  | NULL  | NULL          | g(gc)         | ~             |
| time  | addr  | NULL      | NULL  | LINE  | CLASS_NAME_ID | u(use)        | ~             |
| time  | NULL  | NULL      | EID   | NULL  | NULL          | e(event)      | ~             |
| time  | holder| ref       | cid   | LINE  | CLASS_NAME_ID | c(container)  | ~             | cop       | 
'''
# cursor.execute('''--sql
# CREATE TABLE IF NOT EXISTS TIMELINE(
#     TIME BIGIT,
#     ADDR0 BIGINT,
#     ADDR1 BININT,
#     ID INT,
#     LINE INT,
#     CLASS_NAME_ID INT,
#     TYPE CHAR,
#     TID INTEGER PRIMARY KEY AUTOINCREMENT
#     );''')
def get_files(prefix, pid):
    return glob.glob(f'{prefix}-*-{pid}')

class DataType:
    def __init__(self, values, typech, table):
        self.values = values
        self.typech = ord(typech)
        self.table = table
datatype_map = {
    'gc': DataType('TIME,ADDR0,ADDR1', 'g', 'TIMELINE'),
    'AAStore': DataType('LINE, CLASS_NAME_ID, TIME, ID, ADDR0, ADDR1', 'a', 'TIMELINE'),
    'PutField': DataType('LINE, CLASS_NAME_ID, ID, TIME, ADDR0, ADDR1', 'p', 'TIMELINE'),
    "NEW": DataType('LINE, CLASS_NAME_ID, TIME, ID, ADDR0', 'n', 'TIMELINE'),
}

info_map = {
    'classname': DataType('CLASSNAME', 'c', 'CLASSNAME'),
    'typemap': DataType('ID, TYPE', 't', 'TYPEMAP'),
    'field': DataType('ID, FIELD', 'f', 'FIELD')
}

#for datatype in datatype_map:
#    #print(datatype)
#    files = get_files(datatype, pid)
#    for file in files:
#        with open(file) as f:
#            data = f.readlines()
#        cursor.execute('BEGIN TRANSACTION;')
#        for i, line in enumerate(data):
#            #print(f'INSERT INTO TIMELINE ({datatype_map[datatype].values},TYPE) VALUES({line.strip()}, {datatype_map[datatype].typech})')
#            if line.count(',') != datatype_map[datatype].values.count(','):
#                print(f'[WARNING] Line is broken at {file} line {i} with {line}', flush=True)
#            else:
#                cursor.execute(f'INSERT INTO TIMELINE ({datatype_map[datatype].values},TYPE) VALUES({line.strip()}, {datatype_map[datatype].typech})')
#        cursor.execute('COMMIT;')
# cursor.execute('CREATE INDEX IF NOT EXISTS TIME_INDEX ON TIMELINE(TIME)')
cursor.execute('''CREATE TABLE IF NOT EXISTS CLASSNAME(
    ID INTEGER PRIMARY KEY AUTOINCREMENT,
    CLASSNAME TEXT
);
               ''')
cursor.execute('''CREATE TABLE IF NOT EXISTS TYPEMAP(
    ID INT ,
    TYPE TEXT
);
               ''')
cursor.execute('''CREATE TABLE IF NOT EXISTS FIELD(
    ID INT ,
    FIELD TEXT
);
''')
for info in info_map:
    file_list = get_files(info,pid)
    if len(file_list) == 0:
        continue
    file = get_files(info,pid)[0]
    with open(file) as f:
        data = f.readlines()
        holder = ','.join(['?']*len(info_map[info].values.split(',')))
        print(file)
        for line in data:
            #print(f'INSERT INTO {info_map[info].table} ({holder}) VALUES({line})', *line.split(','))
            #cursor.execute(f'INSERT INTO {info_map[info].table} ({info_map[info].values}) VALUES({line})')
            cursor.execute(f'INSERT INTO {info_map[info].table} ({info_map[info].values}) VALUES({holder})', line.strip().split(','))
##file = get_files('typemap',pid)[0]
##with open(file) as f:
##    data = f.readlines()
##    for line in data:
##        cursor.execute(f'INSERT INTO TYPEMAP ({info_map[datatype].values}) VALUES({line})')
cursor.close()
conn.commit()
conn.close()


