#include "recorder.hpp"
#include <iostream>

#include <map>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <variant>
#include <vector>
#include <filesystem>
#include "fs_util.h"
#include "record_reader.hpp"
using std::map;
using std::string;
using std::unordered_map;
using std::variant;
using std::vector;
using addr_t = uint32_t;
FILE* new_id_file;

constexpr uint32_t GC_MASK = 0x80000000;
struct unique_addr_info
{
    uint32_t id;
    uint64_t time;
};

class addr_map
{
    using obj_id_t = uint32_t;
    unordered_map<addr_t, variant<unique_addr_info, map<uint64_t, obj_id_t>>> _addr_map;
    obj_id_t cur_id = 1;

public:
    addr_map()
    {
        insert_with_id(0, 0, 0);
    }
    obj_id_t query(addr_t addr, uint64_t time)
    {
        auto iter = _addr_map.find(addr);
        if (iter == _addr_map.end())
        {
            return -1;
        }
        unique_addr_info *info = std::get_if<unique_addr_info>(&iter->second);
        if (info)
        {
            return info->id;
        }

        auto &time_map = std::get<1>(iter->second);
        auto iter2 = time_map.upper_bound(time);
        if (iter2 == time_map.begin())
        {
            return -1;
        }
        else
        {
            --iter2;
            return iter2->second;
        }
    }

private:
    void insert_with_id(addr_t addr, uint64_t time, obj_id_t id)
    {
        auto iter = _addr_map.find(addr);
        if (iter == _addr_map.end())
        {
            _addr_map.emplace(addr, unique_addr_info{id, time});
            return;
        }

        unique_addr_info *info = std::get_if<unique_addr_info>(&iter->second);
        if (info)
        {
            unique_addr_info info_bak = *info;
            iter->second = map<uint64_t, obj_id_t>({{info_bak.time, info_bak.id}});
        }

        auto &time_map = std::get<1>(iter->second);
        time_map.emplace(time, id);
    }

public:
    obj_id_t insert(addr_t addr, uint64_t time)
    {
        insert_with_id(addr, time, cur_id++);
        return cur_id-1;
    }
    void insert_gc(addr_t addr, uint64_t time)
    {
        insert_with_id(addr, time, cur_id++ | GC_MASK);
    }

    void insert(addr_t old_addr, addr_t new_addr, uint64_t time)
    {
        obj_id_t id = query(old_addr, time);
        if (id == -1)
        {
            insert_gc(new_addr, time);
        }
        else
        {
            insert_with_id(new_addr, time, id);
        }
    }

    void insert_new_records(NewRecord<0> *buffer, int len)
    {
        for (int i = 0; i < len; ++i)
        {
            auto id = insert(buffer[i].addr, buffer[i].time);
            fprintf(new_id_file, "%d %d %d %d %d\n", id, buffer[i].class_name_id, buffer[i].line, buffer[i].obj_size, buffer[i].obj_typeid);
        }
    }
    void addr2id(addr_t &addr, uint64_t time)
    {
        obj_id_t id = query(addr, time);
        if (id == -1)
        {
            printf("[WARNING] unknown address %p at time %ld", addr, time);
            return;
        }
        addr = id;
    }

    template <typename T>
    void transform(T &val, vector<std::function<addr_t &(T &)>> addr_accessors)
    {
        uint64_t time = val.time;
        for (auto &fn : addr_accessors)
        {
            auto &addr = fn(val);
            addr = query(addr, time);
        }
    }
};

addr_map am;
void process_new_file(const string &filepath)
{
    FILE *f = fopen(filepath.c_str(), "rb");
    constexpr size_t buffer_len = 1 << 10;
    auto buffer = new NewRecord<0>[buffer_len];
    while (true)
    {
        auto len = fread(buffer, sizeof(NewRecord<0>), buffer_len, f);
        if (len <= 0)
        {
            break;
        }
        am.insert_new_records(buffer, len);
    }
    delete[] buffer;
}

void process_gc_file(const string&filepath)
{
    FILE *f = fopen(filepath.c_str(), "rb");
    constexpr size_t buffer_len = 1 << 10;
    auto buffer = new GCRecord<0>[buffer_len];
    while (true)
    {
        auto len = fread(buffer, sizeof(GCRecord<0>), buffer_len, f);
        if (len <= 0)
        {
            break;
        }
        for (int i = 0; i < len; ++i)
        {
            am.insert(buffer[i].org_addr, buffer[i].new_addr, buffer[i].time);
        }
    }
    delete[] buffer;

}

void build_addr_map()
{
    vector<string> new_files;
    filesStartWith(".", new_files, "NEW");
    for (auto &path : new_files)
    {
        process_new_file(path);
    }
    vector<string> gc_files;
    filesStartWith(".", gc_files, "gc");
    for (auto &path: gc_files)
    {
        process_gc_file(path);
    }
}


template <typename T>
void process_data_file(const string &filepath, const vector<std::function<addr_t &(T &)>> &funcs)
{

}
template <typename T>
void process_all_container_data_file(const vector<std::function<addr_t &(T &)>> &funcs)
{
    vector<string> data_files;
    filesStartWith(".", data_files, T::prefix);
    for (const auto &path : data_files)
    {
        std::cout<<path<<std::endl;
        RecordReader<T> reader(path);
        for (const auto& func:funcs)
        {
            reader.register_transformer([&](auto& val) {
                func(val) = am.query(func(val), val.br.time);
            });
        }
        reader.transform_local();
    }
}

template <typename T>
void process_all_field_data_file(const vector<std::function<addr_t &(T &)>> &funcs)
{
    vector<string> data_files;
    filesStartWith(".", data_files, T::prefix);
    for (const auto &path : data_files)
    {
        std::cout<<path<<std::endl;
        RecordReader<T> reader(path);
        for (const auto& func:funcs)
        {
            reader.register_transformer([&](auto& val) {
                func(val) = am.query(func(val), val.time);
            });
        }
        reader.transform_local();
    }
}
int main()
{
    new_id_file = fopen("addr-id.dat", "w");
    build_addr_map();
    process_all_container_data_file(vector<std::function<addr_t&(ListAndSetRecord<0>&)>>{
           [](auto& r) -> addr_t&  {return r.br.holder_addr;},
           [](auto& r) -> addr_t&  {return r.br.ref_addr;},
       });
    process_all_container_data_file(vector<std::function<addr_t&(MapRecord<0>&)>>{
           [](auto& r) -> addr_t&  {return r.key_addr;},
           [](auto& r) -> addr_t&  {return r.value_addr;},
           [](auto& r) -> addr_t&  {return r.br.holder_addr;},
           [](auto& r) -> addr_t&  {return r.br.ref_addr;},
       });
    process_all_field_data_file(vector<std::function<addr_t&(FieldValueRecord<0,0,0,0>&)>>{
           [](auto& r) -> addr_t&  {return r.addr;},
       });
    process_all_field_data_file(vector<std::function<addr_t&(FieldValueRecord<0,0,0,1>&)>>{
           [](auto& r) -> addr_t&  {return r.addr;},
       });
    process_all_field_data_file(vector<std::function<addr_t&(FieldValueRecord<0,0,1,0>&)>>{
           [](auto& r) -> addr_t&  {return r.addr;},
       });
    process_all_field_data_file(vector<std::function<addr_t&(FieldValueRecord<0,0,1,1>&)>>{
           [](auto& r) -> addr_t&  {return r.addr;},
       });
    process_all_field_data_file(vector<std::function<addr_t&(FieldValueRecord<0,1,0,0>&)>>{
           [](auto& r) -> addr_t&  {return r.addr;},
       });
    process_all_field_data_file(vector<std::function<addr_t&(FieldValueRecord<0,1,0,1>&)>>{
           [](auto& r) -> addr_t&  {return r.addr;},
       });
    process_all_field_data_file(vector<std::function<addr_t&(FieldValueRecord<0,1,1,0>&)>>{
           [](auto& r) -> addr_t&  {return r.addr;},
       });
    process_all_field_data_file(vector<std::function<addr_t&(FieldValueRecord<0,1,1,1>&)>>{
           [](auto& r) -> addr_t&  {return r.addr;},
       });
    fclose(new_id_file);
}
