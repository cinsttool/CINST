#include "sqlite3.h"
#include "recorder.hpp"
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include "fs_util.h"
//#include <filesystem>
using namespace std;

constexpr size_t BUF_SIZE = (16L<<30);

#define SQLITE3_EXEC( db, sql, call_back, call_back_args, err_msg)\
{ \
    int rc = sqlite3_exec(db, sql, call_back, call_back_args, err_msg); \
    if (rc != SQLITE_OK) { \
        fprintf(stderr, "Sqlite error at %s:%d with %s", __FILE__, __LINE__, *err_msg); \
        sqlite3_free(*err_msg); \
        exit(-1); \
    }\
} 

//void glob(string path, vector<string>& file, string ext)
//{
//    struct stat s;
//    lstat(path.c_str(), &s);
//    if (!S_ISDIR(s.st_mode)) {
//        fprintf(stderr, "%s is not dir\n", path.c_str());
//        exit(-1);
//    }
//
//    struct dirent* filename;
//    DIR * dir;
//    dir = opendir(path.c_str());
//    if (NULL == dir) {
//        fprintf(stderr, "can not open dir %s\n", path.c_str());
//        exit(-1);
//    }
//
//    while ((filename = readdir(dir)) != NULL) 
//    {
//        if( strcmp( filename->d_name , "." ) == 0 || 
//			strcmp( filename->d_name , "..") == 0    )
//			continue;
//        string namestr = string(filename);
//        if (namestr.length() <= )
//    }
//}
//

void glob(const string& pathstr, vector<string>& files, const string& ext) {
    filesystem::path path(pathstr);
    if (!filesystem::exists(path) || !filesystem::is_directory(path)) {
        fprintf(stderr, "dir %s not exist\n", pathstr.c_str());
    }

    filesystem::directory_iterator dir(path);
    for (auto& ite: dir) {
        if (ite.status().type() == filesystem::file_type::regular) {
            string filename = ite.path().filename().string();
            if (endsWith(filename, ext)) {
                files.push_back(filename);
            }
        }
    }
}

const vector<string> prefix_list = {
    "gc", "AAStore", "PutField", "NEW"
};
unordered_map<string, string> sql_map = {
    {"gc", "TIME,ADDR0,ADDR1"},
    {"AAStore", "LINE, CLASS_NAME_ID, TIME, ID, ADDR0, ADDR1"},
    {"PutField", "LINE, CLASS_NAME_ID, ID, TIME, ADDR0, ADDR1"},
    {"NEW", "LINE, CLASS_NAME_ID, TIME, ID, ADDR0, ADDR1"},
    {"Construct", "LINE, CLASS_NAME_ID, TIME, ID, ADDR0"},
    {"Use", "ID, LINE, CLASS_NAME_ID, ADDR0, TIME"},
    {"Free", "TIME, ADDR0, ADDR1"},
    {"Event", "TIME, ID"}
};

unordered_map<string, string> value_map = {
    {"gc", "?,?,?,?"},
    {"AAStore", "?,?,?,?,?,?,?"},
    {"PutField", "?,?,?,?,?,?,?"},
    {"NEW", "?,?,?,?,?,?,?"},
    {"Construct", "?,?,?,?,?,?"},
    {"Use", "?,?,?,?,?,?"},
    {"Free", "?,?,?,?"},
    {"Event", "?,?,?"}
};
unordered_map<string, char> type_map = {
    {"gc",'g'},
    {"AAStore",'a'},
    {"PutField",'p'},
    {"NEW",'n'},
    {"Construct",'c'},
    {"Use", 'u'},
    {"Free", 'f'},
    {"Event", 'e'}
};

void sql_bind(sqlite3_stmt* pstmt, EventRecord<0> *r) {
    int nCol = 1;
    sqlite3_bind_int64(pstmt, nCol++, r->time);
    sqlite3_bind_int(pstmt, nCol++, r->event_id);
    sqlite3_bind_int(pstmt, nCol++, 'e');
}
void sql_bind(sqlite3_stmt* pstmt, FreeRecord<0> *r) {
    int nCol = 1;
    sqlite3_bind_int64(pstmt, nCol++, r->time);
    sqlite3_bind_int(pstmt, nCol++, r->addr);
    sqlite3_bind_int(pstmt, nCol++, r->obj_size);
    sqlite3_bind_int(pstmt, nCol++, 'f');
}
void sql_bind(sqlite3_stmt* pstmt, UseRecord<0> *r) {
    int nCol = 1;
    sqlite3_bind_int(pstmt, nCol++, r->type);
    sqlite3_bind_int(pstmt, nCol++, r->line);
    sqlite3_bind_int(pstmt, nCol++, r->class_name_id);
    sqlite3_bind_int64(pstmt, nCol++, r->addr);
    sqlite3_bind_int64(pstmt, nCol++, r->time);
    sqlite3_bind_int(pstmt, nCol++, 'u');
}
void sql_bind(sqlite3_stmt* pstmt, GCRecord<0>* r) {
    int nCol = 1;
    sqlite3_bind_int64(pstmt, nCol++, r->time);
    sqlite3_bind_int64(pstmt, nCol++, r->org_addr);
    sqlite3_bind_int64(pstmt, nCol++, r->new_addr);
    sqlite3_bind_int(pstmt, nCol++, 'g');
}

void sql_bind(sqlite3_stmt* pstmt, AAStoreRecord<0>* r) {
    int nCol = 1;
    sqlite3_bind_int(pstmt, nCol++, r->line);
    sqlite3_bind_int(pstmt, nCol++, r->class_name_id);
    sqlite3_bind_int64(pstmt, nCol++, r->time);
    sqlite3_bind_int64(pstmt, nCol++, r->index);
    sqlite3_bind_int64(pstmt, nCol++, r->holder_addr);
    sqlite3_bind_int64(pstmt, nCol++, r->ref_addr);
    sqlite3_bind_int(pstmt, nCol++, 'a');
}
void sql_bind(sqlite3_stmt* pstmt, NewRecord<0>* r) {
    int nCol = 1;
    sqlite3_bind_int(pstmt, nCol++, r->line);
    sqlite3_bind_int(pstmt, nCol++, r->class_name_id);
    sqlite3_bind_int64(pstmt, nCol++, r->time);
    sqlite3_bind_int64(pstmt, nCol++, r->obj_typeid);
    sqlite3_bind_int64(pstmt, nCol++, r->addr);
    sqlite3_bind_int64(pstmt, nCol++, r->obj_size);
    sqlite3_bind_int(pstmt, nCol++, 'n');
}
void sql_bind(sqlite3_stmt* pstmt, ConstructRecord<0>* r) {
    int nCol = 1;
    sqlite3_bind_int(pstmt, nCol++, r->line);
    sqlite3_bind_int(pstmt, nCol++, r->class_name_id);
    sqlite3_bind_int64(pstmt, nCol++, r->time);
    sqlite3_bind_int64(pstmt, nCol++, r->obj_typeid);
    sqlite3_bind_int64(pstmt, nCol++, r->addr);
    sqlite3_bind_int(pstmt, nCol++, 'c');
}
void sql_bind(sqlite3_stmt* pstmt, PutFieldRecord<0>* r) {
    int nCol = 1;
    sqlite3_bind_int(pstmt, nCol++, r->line);
    sqlite3_bind_int(pstmt, nCol++, r->class_name_id);
    sqlite3_bind_int(pstmt, nCol++, r->field_id);
    sqlite3_bind_int64(pstmt, nCol++, r->time);
    sqlite3_bind_int64(pstmt, nCol++, r->holder_addr);
    sqlite3_bind_int64(pstmt, nCol++, r->ref_addr);
    sqlite3_bind_int(pstmt, nCol++, 'p');
}

template<typename Record>
void load_timeline(sqlite3* db, char* _, stringstream& ss,  FILE* fin, char* buf) {
    char* err_msg = NULL;
    char* pzTail;
    static sqlite3_stmt *pstmt = nullptr;
    if (pstmt == nullptr) {
        string ssql = string("INSERT INTO TIMELINE(")+sql_map[Record::prefix]+",TYPE) VALUES("+value_map[Record::prefix]+")";
        const char* sql = ssql.c_str();
        sqlite3_prepare_v2(db, sql, strlen(sql), &pstmt, NULL);
    }

    while (true) {
        size_t size = fread(buf, sizeof(Record), BUF_SIZE/sizeof(Record), fin);
        if (size <= 0) {
            return;
        }
        for (size_t i = 0; i < size; ++i) {
            //ss.str("");
            //ss<<"INSERT INTO TIMELINE ("<<sql_map[string(Record::prefix)]<<",TYPE"<<") VALUES("<<((Record*)buf)[i]<<","<<(int)type_map[Record::prefix]<<")";
            //printf("%s\n", ss.str().c_str()); 
            //SQLITE3_EXEC(db, ss.str().c_str(), NULL, NULL, &err_msg);
            sqlite3_reset(pstmt);
            sql_bind(pstmt, ((Record*)buf)+i);
            sqlite3_step(pstmt);
        }
    }
}

//template <>
//void load_timeline<GCRecord>(sqlite3* db, char* sql, stringstream& ss,  FILE* fin, char* buf)
//{
//    char* err_msg = NULL;
//    char* pzTail;
//    sqlite3_stmt *pstmt;
//    auto sql = "INSERT INTO TIMELINE("+sql_map[string(GCRecord::prefix)]+",TYPE) VALUES(?,?,?,?)";
//    sqlite3_prepare_v2(db, sql, strlen(sql), &pstmt, &pzTail);
//    while (true) {
//        size_t size = fread(buf, sizeof(GCRecord), BUF_SIZE/sizeof(GCRecord), fin);
//        if (size <= 0) {
//            return;
//        }
//        for (size_t i = 0; i < size; ++i) {
//            int nCol = 1;
//            GCRecord* r = ((GCRecord*)buf)+i;
//            //ss.str("");
//            //ss<<"INSERT INTO TIMELINE ("<<sql_map[string(GCRecord::prefix)]<<",TYPE"<<") VALUES("<<((Record*)buf)[i]<<","<<(int)type_map[Record::prefix]<<")";
//            ////printf("%s\n", ss.str().c_str()); 
//            //SQLITE3_EXEC(db, ss.str().c_str(), NULL, NULL, &err_msg);
//        }
//    }
//
//}

//template<typename... Ts>
//void call_load_timeline(const string& filename, Ts&... args) {
//}

#define LOAD_TIMELINE(type, filename, ...) \
if (startsWith(filename, type::prefix)) { \
    load_timeline<type>(__VA_ARGS__); \
}

template<typename T, typename... Ts>
void call_load_timeline(const string& filename, Ts && ... args) {
    if (startsWith(filename, T::prefix)) {
        load_timeline<T>(filename, std::forward<Ts>(args)...);
    }
}
//template<typename T, typename... T1, typename... Ts>
//void call_load_timeline(const string& filename, Ts && ... args) {
//    call_load_timeline<T, Ts...>(filename, std::forward<Ts>(args)...);
//    call_load_timeline<T1..., Ts...>(filename, std::forward<Ts>(args)...);
//}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "csv2db <pid>\n");
        exit(-1);
    }
    sqlite3 *db = NULL;
    string pid = string(argv[1]);
    string db_name = pid + ".db";

    char* err_msg = NULL;
    int rc = sqlite3_open(db_name.c_str(), &db);
    //int rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open db %s: %s\n", db_name.c_str(), sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(-1);
    }
    sqlite3_exec(db, "PRAGMA synchronous = OFF", 0, 0, 0);

    const char * create_timeline_sql =  \
    "CREATE TABLE IF NOT EXISTS TIMELINE("
        "TIME BIGIT,"
        "ADDR0 BIGINT,"
        "ADDR1 BININT,"
        "ID INT,"
        "LINE INT,"
        "CLASS_NAME_ID INT,"
        "TYPE CHAR,"
        "TID INTEGER PRIMARY KEY AUTOINCREMENT"
    ");";

    SQLITE3_EXEC(db, create_timeline_sql, NULL, NULL, &err_msg);
    SQLITE3_EXEC(db, "BEGIN", NULL, NULL, &err_msg);
    char* buf = new char[BUF_SIZE];
    char* sql = new char[4096];
    vector<string> files;
    glob(".",files, pid);
    
    for (auto& file: files) {
        FILE* fin = fopen(file.c_str(), "rb");
        stringstream ss;
        //if (startsWith(file, "gc")) {
        //    load_timeline<GCRecord>(db, sql, ss, fin, buf);
        //}
        //else if (startsWith(file, "AAStore")) {
        //    load_timeline<AAStoreRecord>(db, sql, ss, fin, buf);
        //}

        //call_load_timeline<GCRecord>(file, db, sql, ss, fin, buf);
        LOAD_TIMELINE(GCRecord<0>, file, db, sql, ss, fin, buf);
        LOAD_TIMELINE(AAStoreRecord<0>, file, db, sql, ss, fin, buf);
        LOAD_TIMELINE(PutFieldRecord<0>, file, db, sql, ss, fin, buf);
        LOAD_TIMELINE(NewRecord<0>, file, db, sql, ss, fin, buf);
        LOAD_TIMELINE(ConstructRecord<0>, file, db, sql, ss, fin, buf);
        LOAD_TIMELINE(UseRecord<0>, file, db, sql, ss, fin, buf);
        LOAD_TIMELINE(FreeRecord<0>, file, db, sql, ss, fin, buf);
        LOAD_TIMELINE(EventRecord<0>, file, db, sql, ss, fin, buf);

        printf("finish %s\n", file.c_str());
    }

    SQLITE3_EXEC(db, "COMMIT", NULL, NULL, &err_msg);
    
    sqlite3_close(db);

}
