#pragma once

#include <sys/types.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <filesystem>
template<int is_64bit> struct AddressType;

template<>
struct AddressType<1>
{
    using value = uint64_t;
};
template<>
struct AddressType<0>
{
    using value = uint32_t;
};


inline uint64_t nanotime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC , &ts);
    return ts.tv_sec*((uint64_t)1e9)+ts.tv_nsec;
}

template<int is_64bit>
struct GCRecord {
    static constexpr const char* prefix = "gc";
    using addr_t = typename AddressType<is_64bit>::value;
    addr_t org_addr;
    addr_t new_addr;
    uint64_t time;
};
template<int is_64bit>
std::ostream& operator<<(std::ostream& os, const GCRecord<is_64bit>& r) {
    os<<r.time<<","<<r.org_addr<<","<<r.new_addr<<std::endl;
    return os;
}

template<int is_64bit>
struct PutFieldRecord
{
    static constexpr const char* prefix = "PutField";
    using addr_t = typename AddressType<is_64bit>::value;
    uint16_t line;
    uint16_t class_name_id;
    int field_id;
    addr_t holder_addr;
    addr_t ref_addr;
    uint64_t time;
};
template<int is_64bit>
std::ostream& operator <<(std::ostream& os, const PutFieldRecord<is_64bit>& r) {
    os<<r.line<<","<<r.class_name_id<<","<<r.field_id<<","<<r.time<<","<<r.holder_addr<<","<<r.ref_addr<<"\n";
    return os;
}

template<int is_64bit>
struct ConstructRecord
{
    static constexpr const char* prefix = "Construct";
    using addr_t = typename AddressType<is_64bit>::value;
    uint16_t line;
    uint16_t class_name_id;
    int obj_typeid;
    addr_t addr;
    uint64_t time;
};

template <int is_64bit>
std::ostream& operator <<(std::ostream& os, const ConstructRecord<is_64bit>& r) {
    os<<r.line<<","<<r.class_name_id<<","<<r.time<<","<<r.obj_typeid<<","<<r.addr<<"\n";
    return os;
}
template<int is_64bit>
struct NewRecord
{
    static constexpr const char* prefix = "NEW";
    using addr_t = typename AddressType<is_64bit>::value;
    uint16_t line;
    uint16_t class_name_id;
    int obj_typeid;
    uint32_t obj_size;
    addr_t addr;
    uint64_t time;
};
template <int is_64bit>
std::ostream& operator <<(std::ostream& os, const NewRecord<is_64bit>& r) {
    os<<r.line<<","<<r.class_name_id<<","<<r.time<<","<<r.obj_typeid<<","<<r.addr<<","<<r.obj_size<<"\n";
    return os;
}

template<int is_64bit>
struct AAStoreRecord
{
    static constexpr const char* prefix = "AAStore";
    using addr_t = typename AddressType<is_64bit>::value;
    uint16_t line;
    uint16_t class_name_id;
    int index;
    addr_t holder_addr;
    addr_t ref_addr;
    uint64_t time;
};
template<int is_64bit>
std::ostream& operator <<(std::ostream& os, const AAStoreRecord<is_64bit>& r) {
    os<<r.line<<","<<r.class_name_id<<","<<r.time<<","<<r.index<<","<<r.holder_addr<<","<<r.ref_addr<<"\n";
    return os;
}

template<int is_64bit>
struct UseRecord
{
    static constexpr const char* prefix = "Use";
    using addr_t = typename AddressType<is_64bit>::value;
    uint16_t type;
    uint16_t line;
    uint16_t class_name_id;
    addr_t addr;
    uint64_t time;
};
template<int is_64bit>
std::ostream& operator <<(std::ostream& os, const UseRecord<is_64bit>& r) {
    os<<r.type<<","<<r.line<<","<<r.class_name_id<<","<<r.addr<<","<<r.time<<"\n";
    return os;
}

template<bool is_put, bool val_len, bool is_float, bool is_static, bool is_string>
constexpr const char* concate() {
    if(is_string) {
        int id = (is_static?10:0)+(is_put?1:0);
        switch(id) {
            case 11: return "PutStaticString";
            case 10: return "GetStaticString";
            case 1: return "PutStringField";
            case 0: return "GetStringField";
            default: return "ERROR";
        }
    }
    int id = (is_put?100:0)+(val_len?10:0)+(is_float?1:0);
    if(!is_static) {
        switch(id) {
            case 100: return "FieldValue100";
            case 110: return "FieldValue110";
            case 101: return "FieldValue101";
            case 111: return "FieldValue111";
            case 0: return "FieldValue000";
            case 10: return "FieldValue010";
            case 1: return "FieldValue001";
            case 11: return "FieldValue011";
            default: return "FieldValue";
        }
    }
    else {
        switch(id) {
            case 100: return "StaticFieldValue100";
            case 110: return "StaticFieldValue110";
            case 101: return "StaticFieldValue101";
            case 111: return "StaticFieldValue111";
            case 0: return "StaticFieldValue000";
            case 10: return "StaticFieldValue010";
            case 1: return "StaticFieldValue001";
            case 11: return "StaticFieldValue011";
            default: return "StaticFieldValue";
        }
    }
}

// type: 0: get, 1: put, val_len: 0 : 32, 1 : 64
template<int is_64bit, bool is_put, bool val_len, bool is_float>
struct FieldValueRecord
{
    static constexpr const char* prefix = concate<is_put, val_len, is_float, false, false>();
    using addr_t = typename AddressType<is_64bit>::value;
    uint16_t line;
    uint16_t class_name_id;
    addr_t addr;
    uint64_t time;
    uint16_t field_id;
    using val_t = typename AddressType<val_len>::value;
    val_t value;
};
template<int is_64bit, bool is_put, bool val_len, bool is_float>
std::ostream& operator <<(std::ostream& os, const FieldValueRecord<is_64bit, is_put, val_len, is_float>& r) {
    if(is_put) os<<"Put,";
    else os <<"Get,";
    if(val_len) {
        if(is_float) {
            os<<(double)r.value<<",";
        } else {
            os<<(int64_t)r.value<<",";
        }
    } else {
        if(is_float) {
            os<<(float)r.value<<",";
        } else {
            os<<(int32_t)r.value<<",";
        }
    }
    os<<r.line<<","<<r.class_name_id<<","<<r.addr<<","<<r.field_id<<","<<r.time<<"\n";
    return os;
}

// type: 0: get, 1: put, val_len: 0 : 32, 1 : 64
template<int is_64bit, bool is_put, bool val_len, bool is_float>
struct StaticFieldValueRecord
{
    static constexpr const char* prefix = concate<is_put, val_len, is_float, true, false>();
    uint16_t line;
    uint16_t class_name_id;
    uint64_t time;
    uint16_t field_id;
    using val_t = typename AddressType<val_len>::value;
    val_t value;
};
template<int is_64bit, bool is_put, bool val_len, bool is_float>
std::ostream& operator <<(std::ostream& os, const StaticFieldValueRecord<is_64bit, is_put, val_len, is_float>& r) {
    if(is_put) os<<"PutStatic,";
    else os <<"GetStatic,";
    if(val_len) {
        if(is_float) {
            os<<(double)r.value<<",";
        } else {
            os<<(int64_t)r.value<<",";
        }
    } else {
        if(is_float) {
            os<<(float)r.value<<",";
        } else {
            os<<(int32_t)r.value<<",";
        }
    }
    os<<r.line<<","<<r.class_name_id<<","<<r.field_id<<","<<r.time<<"\n";
    return os;
}

// is_put: 0: get, 1: put
template<int is_64bit, bool is_put>
struct StringFieldValueRecord
{
    static constexpr const char* prefix = concate<is_put, false, false, false, true>();
    using addr_t = typename AddressType<is_64bit>::value;
    uint16_t line;
    uint16_t class_name_id;
    addr_t addr;
    uint64_t time;
    uint16_t field_id;
    uint16_t string_id;
};
template<int is_64bit, bool is_put>
std::ostream& operator <<(std::ostream& os, const StringFieldValueRecord<is_64bit, is_put>& r) {
    if(is_put) os<<"Put,";
    else os <<"Get,";
    os<<r.string_id<<","<<r.line<<","<<r.class_name_id<<","<<r.addr<<","<<r.field_id<<","<<r.time<<"\n";
    return os;
}

// is_put: 0: get, 1: put
template<int is_64bit, bool is_put>
struct StaticStringFieldValueRecord
{
    static constexpr const char* prefix = concate<is_put, false, false, true, true>();
    uint16_t line;
    uint16_t class_name_id;
    uint64_t time;
    uint16_t field_id;
    uint16_t string_id;
};
template<int is_64bit, bool is_put>
std::ostream& operator <<(std::ostream& os, const StaticStringFieldValueRecord<is_64bit, is_put>& r) {
    if(is_put) os<<"PutStatic,";
    else os <<"GetStatic,";
    os<<r.string_id<<","<<r.line<<","<<r.class_name_id<<","<<r.field_id<<","<<r.time<<"\n";
    return os;
}

template<int is_64bit>
struct ContainerRecord
{
    using addr_t = typename AddressType<is_64bit>::value;
    addr_t holder_addr;
    addr_t ref_addr;
    uint16_t cid;
    uint16_t copid;
    uint64_t time;
    uint32_t size;
};

template<int is_64bit>
struct ListAndSetRecord
{
    static constexpr const char* prefix = "List-Set";
    ContainerRecord<is_64bit> br;
    // add(obj)'Z', add('I'obj)V or get('I')obj
    uint32_t idx_or_succ;
};
template<int is_64bit>
std::ostream& operator <<(std::ostream& os, const ListAndSetRecord<is_64bit>& r) {
    os<<r.br.cid<<","
      <<r.br.copid<<","
      <<r.br.holder_addr<<","
      <<r.br.ref_addr<<","
      <<r.idx_or_succ<<","
      <<r.br.time<<","
      <<r.br.size<<"\n";
    return os;
}

template<int is_64bit>
struct MapRecord
{
    static constexpr const char* prefix = "Map";
    ContainerRecord<is_64bit> br;
    using addr_t = typename AddressType<is_64bit>::value;
    addr_t value_addr;
    addr_t key_addr;
};
template<int is_64bit>
std::ostream& operator <<(std::ostream& os, const MapRecord<is_64bit>& r) {
    os<<r.br.cid<<","
      <<r.br.copid<<","
      <<r.br.holder_addr<<","
      <<r.br.ref_addr<<","
      <<r.key_addr<<","
      <<r.value_addr<<","
      <<r.br.time<<","
      <<r.br.size<<"\n";
    return os;
}

struct ContainerLineInfoRecord
{
    static constexpr const char* prefix = "ContainerLineInfo";
    uint16_t line;
    uint16_t class_name_id;
    uint64_t time;
};

std::ostream& operator <<(std::ostream& os, const ContainerLineInfoRecord& r) {
    os<<r.line<<","<<r.class_name_id<<","<<r.time<<"\n";
    return os;
}

template<int is_64bit>
struct FreeRecord
{
    static constexpr const char* prefix = "Free";
    using addr_t = typename AddressType<is_64bit>::value;
    uint32_t obj_size;
    addr_t addr;
    uint64_t time;
};

template<int is_64bit>
std::ostream& operator <<(std::ostream& os, const FreeRecord<is_64bit>& r) {
    os<<r.addr<<","<<r.time<<"\n";
    return os;
}


template<int is_64bit>
struct EventRecord
{
    enum Event {
        GC_START_EVENT = 1,
        GC_END_EVENT = 2,
    };
    static constexpr const char* prefix= "Event";
    int event_id;
    uint64_t time;
};
template<int is_64bit>
std::ostream& operator <<(std::ostream& os, const EventRecord<is_64bit>& r) {
    os<<r.time<<","<<r.event_id<<"\n";
    return os;
}

template<typename Record>
class Recorder
{
    int _limit;
    int _pos;
    pid_t _tid;
    pid_t _pid;
    std::ofstream* fout;
    std::ofstream* fstrout;
    Record *_buffer;
    std::string _prefix;

public:
    Recorder(int limit, int tid, int pid) : _limit(limit), _pos(0), _tid(tid), _pid(pid), fout(nullptr)
    {
        static_assert(std::is_trivial<Record>::value);
        _buffer = new Record[_limit];
        _prefix = std::string{Record::prefix}+"-";
        fstrout = NULL;
    }
    void sync_csv()
    {
        if (!fout) {
            std::filesystem::path path(std::string("data-")+std::to_string(_pid));
            if (!std::filesystem::exists(path)) {
                std::filesystem::create_directory(path);
            }
            fout = new std::ofstream();
            // printf("%s\n", (_prefix + std::to_string(_tid) + "-" + std::to_string(_pid)).c_str());
            fout->open(path.string() + "/" + _prefix + std::to_string(_tid) + "-" + std::to_string(_pid), std::ios::out|std::ios::binary);
        }
        for (int i = 0; i < _pos; ++i) 
        {
            *fout<<_buffer[i];
        }
        // fout->flush();
        _pos = 0;
    }
    void sync_bin()
    {
        if (!fout) {
            std::filesystem::path path(std::string("data-")+std::to_string(_pid));
            if (!std::filesystem::exists(path)) {
                std::filesystem::create_directory(path);
            }
            fout = new std::ofstream();
            fout->open(path.string() + "/" + _prefix + std::to_string(_tid) + "-" + std::to_string(_pid), std::ios::out|std::ios::binary);
        }
        //for (int i = 0; i < _pos; ++i) 
        //{
        //    *fout<<_buffer[i];
        //}
        fout->write((const char*)_buffer, sizeof(Record) * _pos);
        // fout->flush();
        _pos = 0;
    }
    void sync() {
        #ifdef __CSV_OUTPUT__
        sync_csv();
        #else
        sync_bin();
        #endif
    }
    ~Recorder()
    {
        if (_pos != 0) 
        {
            sync();
        }
        if (fout) {
            fout->close();
            delete fout;
        }
        if (fstrout) {
            fstrout->close();
            delete fstrout;
        }
    }
    void add(const Record& r) 
    {
        if (_pos >= _limit)
        {
            sync();
        }
        _buffer[_pos++] = r;
    }
    void addString(std::string& str)
    {
        if (!fstrout) {
            std::filesystem::path path(std::string("data-")+std::to_string(_pid));
            if (!std::filesystem::exists(path)) {
                std::filesystem::create_directory(path);
            }
            fstrout = new std::ofstream();
            // we have different recorder for put/get static/non type string, so we should add _prefix to separate the stream.
            fstrout->open(path.string() + "/" + _prefix + "string-" + std::to_string(_tid) + "-" + std::to_string(_pid), std::ios::out|std::ios::binary);
        }
        *fstrout<<str;
    }
};