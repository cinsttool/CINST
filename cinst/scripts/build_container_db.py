#!/usr/bin/env python3
import sqlite3
import os
import glob
pid = os.path.abspath('.').split('-')[-1]
int(pid)
#pid = 58371
dbname = f'{pid}.db'


'''
CONTAINERS
| 32            | 32        | 16    | 16    | 64    | 32    | 32| 32| 16    | 16            |
| holder_addr   | ref_addr  | cid   | copid | time  | size  | v1| v2| line  | class_name_id |
'''

from ctypes import *

container_record_fields = [("holder_addr", c_uint32),
               ("ref_addr", c_uint32),
               ("cid", c_uint16),
               ("copid", c_uint16),
               ("time", c_uint64),
               ("size", c_uint32)
               ]

class ContainerRecord(Structure):
    _fields_ = container_record_fields
    def data4sql(self):
        return [self.holder_addr, self.ref_addr, self.cid, self.copid, 
                self.time, self.size]


line_info_fields = [("line", c_uint16),
                    ("class_name_id", c_uint16),
                    ("time", c_uint64)]

class ContainerLineInfoRecord(Structure):
    _fields_ = line_info_fields

    def data4sql(self):
        return [self.line, self.class_name_id]

list_and_set_fields = [("br", ContainerRecord)] + [("idx_or_succ", c_uint32)]

class ListAndSetRecord(Structure):
    _fields_ = list_and_set_fields

    def data4sql(self):
        br = self.br
        return br.data4sql() + [self.idx_or_succ, -1]
map_fields = [("br", ContainerRecord), ("value_addr", c_uint32), ("key_addr", c_uint32)]

class MapRecord(Structure):
    _fields_ = map_fields
    def data4sql(self):
        return self.br.data4sql() + [self.value_addr, self.key_addr]


        

conn = sqlite3.connect(dbname)

cursor = conn.cursor()

cursor.execute('''--sql
CREATE TABLE IF NOT EXISTS CONTAINERS(
    HOLDER_ID INT,
    REF_ID INT,
    CID SHORT,
    COPID SHORT,
    TIME BIGINT,
    SIZE INT,
    V1 INT,
    V2 INT,
    LINE SHORT,
    CLASS_NAME_ID SHORT
    );''')


list_set_files = glob.glob(f'List-Set-*-{pid}')
line_files = glob.glob(f'ContainerLineInfo-*-{pid}')
map_files = glob.glob(f'Map-*-{pid}')
print(line_files)




class RecordBuffer:
    def __init__(self):
        self.line_info_records = []
        self.container_info_records = []
    
    '''
    if new_record.level = 0 and queue is not empty
        arrange queue
        new_record -> queue
        return arranged queue
    else
        new_record -> queue 
    '''
    def push(self, line_info_record, container_info_record):
        if line_info_record.container_filter_flag == 0 and len(self.line_info_records) != 0:
            res = self.pop()
            self.line_info_records = [line_info_record]
            self.container_info_records = [container_info_record]
            return res
        else:
            self.line_info_records.append(line_info_record)
            self.container_info_records.append(container_info_record)
            return []

    def pop(self):
        res = zip(self.line_info_records, reversed(self.container_info_records))
        self.line_info_records = []
        self.container_info_records = []
        return res
        

class ContainerBuffer():
    def __init__(self, buf, struct):
        self.buf = buf
        self.pos = -1
        self.struct = struct
        self.top = None
        self.next()

    
    def cur(self):
        return self.top
    def next(self):
        self.pos += 1
        if self.pos * sizeof(self.struct) + sizeof(self.struct)-1 >= len(self.buf):
            self.top = None
            return self.top
        self.top = self.struct.from_buffer_copy(self.buf, self.pos * sizeof(self.struct))
        return self.top
class ContainerBufferSelector():
    def __init__(self, *bufs):
        self.bufs = [ContainerBuffer(*buf) for buf in bufs if None not in buf]
    def read(self):
        target_bufs = [x for x in self.bufs if x.cur() is not None]
        if len(target_bufs) == 0:
            return None
        target_buf = min(target_bufs, key=lambda x: x.cur().br.time)
        res = target_buf.cur()
        target_buf.next()
        return res
        

def records(line_info_buf, list_and_set_buf, map_buf):
    record_buffer = RecordBuffer()
    counter = 0
    container_buffer_selector = ContainerBufferSelector((list_and_set_buf, ListAndSetRecord), (map_buf, MapRecord))
    if list_and_set_buf is None and map_buf is None:
        print('[WARNING] only line info records')
        return
        
    next_r1 = container_buffer_selector.read()
    # print(f"list: {len(list_and_set_buf)//sizeof(ListAndSetRecord)} map: {len(map_buf)//sizeof(MapRecord)} line: {len(line_info_buf)/sizeof(ContainerLineInfoRecord)}")
    for i in range(len(line_info_buf)// sizeof(ContainerLineInfoRecord)):
        r1 = next_r1
        if r1 == None:
            print('[WARNING] number of container records is less than line info records')
            return

        # r1 = ListAndSetRecord.from_buffer_copy(list_and_set_buf, i*sizeof(ListAndSetRecord))
        r2 = ContainerLineInfoRecord.from_buffer_copy(line_info_buf, i*sizeof(ContainerLineInfoRecord))
        if r1.br.time > r2.time:
            print(f'[WARNING] time of container_record {r1.br.time} is larger than line_info_record {r2.time}.')
            continue

        next_r1 = container_buffer_selector.read()
        while next_r1 is not None and r2.time - r1.br.time > r2.time - next_r1.br.time and next_r1.br.time < r2.time:
            r1 = next_r1
            next_r1 = container_buffer_selector.read()
        res = r1.data4sql() + r2.data4sql()
        yield res
    
    
    
    
    
for line_filename in line_files:
    tid = line_filename.split('-')[1]
    print(tid)
    list_set_filename = f'List-Set-{tid}-{pid}'
    map_filename = f'Map-{tid}-{pid}'
    def read_if_exists(filename):
        if os.path.exists(filename):
            with open(filename, 'rb') as f:
                return f.read()
        else:
            return None
    line_buf = read_if_exists(line_filename)
    list_set_buf = read_if_exists(list_set_filename)
    map_buf = read_if_exists(map_filename)

    cursor.executemany(
        '''INSERT INTO CONTAINERS(HOLDER_ID, REF_ID, CID, COPID, TIME, SIZE, V1, V2, LINE, CLASS_NAME_ID) VALUES (?,?,?,?,?,?,?,?,?,?)''',
        records(line_buf, list_set_buf, map_buf)
    )
cursor2 = conn.cursor()
cursor2.execute('''CREATE TABLE IF NOT EXISTS ALLOCATIONS (
    ID INT,
    CLASS_NAME_ID INT,
    LINE INT,
    INIT_SIZE INT,
    TYPEID INT
    )''')
with open('addr-id.dat') as f:
    data = f.readlines()
    data_nums = [[int(x) for x in y.split(' ')] for y in data]
    cursor2.executemany(
        'INSERT INTO ALLOCATIONS(ID,CLASS_NAME_ID, LINE, INIT_SIZE, TYPEID) VALUES (?,?,?,?,?)',
        data_nums
    )
cursor3 = conn.cursor()
cursor3.execute('''CREATE TABLE IF NOT EXISTS CONTAINER_OP (
    COP INT,
    COP_NAME TEXT
)''')
for file in glob.glob('containerOp-*'):
    with open(file) as f:
        data = f.readlines()
        data = [x.strip().split(',') for x in data]
        data = [(int(x[0]), x[1]) for x in data]
        cursor3.executemany('''INSERT INTO CONTAINER_OP(COP,COP_NAME) VALUES(?,?)''', data)

conn.commit()
