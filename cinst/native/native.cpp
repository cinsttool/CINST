#include "logger_Record.h"
#include "recorder.hpp"
#include <sys/syscall.h>
#include <unistd.h>
#include <unordered_map>
#include <spdlog/spdlog.h>


thread_local int* args_buffer = nullptr;

constexpr int LIMIT=(1<<12);
thread_local bool enablePrintNEW = true; // enable printNEW or not?

// TODO: allcote the _buffer for specific Recorder when necessary!
thread_local std::unordered_map<std::string, int> stringMap;
thread_local Recorder<PutFieldRecord<0>> putFieldRecoreder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<NewRecord<0>> newRecoreder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<AAStoreRecord<0>> aaStoreRecoreder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<UseRecord<0>> useRecorder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<FreeRecord<0>> freeRecorder(LIMIT, syscall(SYS_gettid), getpid());

// for container mode 
// for recursion: e.g. HashSet will call HashMap op
thread_local bool is_right_after = false;
thread_local bool enable = true; // enable record or not?
thread_local Recorder<ListAndSetRecord<0>> listAndSetRecord(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<MapRecord<0>> mapRecord(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<ContainerLineInfoRecord> containerLineInfoRecorder(LIMIT, syscall(SYS_gettid), getpid());

// non-static
// float
// getfield value 32bit
thread_local Recorder<FieldValueRecord<0,0,0,1>> fieldValueRecord0001(LIMIT, syscall(SYS_gettid), getpid());
// getfield value 64bit
thread_local Recorder<FieldValueRecord<0,0,1,1>> fieldValueRecord0011(LIMIT, syscall(SYS_gettid), getpid());
// putfield value 32bit
thread_local Recorder<FieldValueRecord<0,1,0,1>> fieldValueRecord0101(LIMIT, syscall(SYS_gettid), getpid());
// putfield value 64bit
thread_local Recorder<FieldValueRecord<0,1,1,1>> fieldValueRecord0111(LIMIT, syscall(SYS_gettid), getpid());

// integer
// getfield value 32bit
thread_local Recorder<FieldValueRecord<0,0,0,0>> fieldValueRecord0000(LIMIT, syscall(SYS_gettid), getpid());
// getfield value 64bit
thread_local Recorder<FieldValueRecord<0,0,1,0>> fieldValueRecord0010(LIMIT, syscall(SYS_gettid), getpid());
// putfield value 32bit
thread_local Recorder<FieldValueRecord<0,1,0,0>> fieldValueRecord0100(LIMIT, syscall(SYS_gettid), getpid());
// putfield value 64bit
thread_local Recorder<FieldValueRecord<0,1,1,0>> fieldValueRecord0110(LIMIT, syscall(SYS_gettid), getpid());

// String
// getfield
thread_local Recorder<StringFieldValueRecord<0,0>> stringFieldValueRecord00(LIMIT, syscall(SYS_gettid), getpid());
// putfield
thread_local Recorder<StringFieldValueRecord<0,1>> stringFieldValueRecord01(LIMIT, syscall(SYS_gettid), getpid());

// static
// float
// getstaticfield value 32bit
thread_local Recorder<StaticFieldValueRecord<0,0,0,1>> staticFieldValueRecord0001(LIMIT, syscall(SYS_gettid), getpid());
// getstaticfield value 64bit
thread_local Recorder<StaticFieldValueRecord<0,0,1,1>> staticFieldValueRecord0011(LIMIT, syscall(SYS_gettid), getpid());
// putstaticfield value 32bit
thread_local Recorder<StaticFieldValueRecord<0,1,0,1>> staticFieldValueRecord0101(LIMIT, syscall(SYS_gettid), getpid());
// putstaticfield value 64bit
thread_local Recorder<StaticFieldValueRecord<0,1,1,1>> staticFieldValueRecord0111(LIMIT, syscall(SYS_gettid), getpid());

// integer
// getstaticfield value 32bit
thread_local Recorder<StaticFieldValueRecord<0,0,0,0>> staticFieldValueRecord0000(LIMIT, syscall(SYS_gettid), getpid());
// getstaticfield value 64bit
thread_local Recorder<StaticFieldValueRecord<0,0,1,0>> staticFieldValueRecord0010(LIMIT, syscall(SYS_gettid), getpid());
// putstaticfield value 32bit
thread_local Recorder<StaticFieldValueRecord<0,1,0,0>> staticFieldValueRecord0100(LIMIT, syscall(SYS_gettid), getpid());
// putstaticfield value 64bit
thread_local Recorder<StaticFieldValueRecord<0,1,1,0>> staticFieldValueRecord0110(LIMIT, syscall(SYS_gettid), getpid());

// String
// getfield
thread_local Recorder<StaticStringFieldValueRecord<0,0>> staticStringFieldValueRecord00(LIMIT, syscall(SYS_gettid), getpid());
// putfield
thread_local Recorder<StaticStringFieldValueRecord<0,1>> staticStringFieldValueRecord01(LIMIT, syscall(SYS_gettid), getpid());

/*
 * Class:     logger_Record
 * Method:    addPutFieldRecord
 * Signature: (JJII)V
 */
JNIEXPORT void JNICALL Java_logger_Record_addPutFieldRecord
  (JNIEnv *jvmti_env, jclass klass, jlong holder_addr, jlong ref_addr, jint line, jint class_name_id, int field_id) {
    PutFieldRecord<0> r;
    r.time = nanotime();
    r.holder_addr = holder_addr>>3;
    r.ref_addr = ref_addr>>3;
    r.field_id = field_id;
    r.line = line;
    r.class_name_id = class_name_id;
    putFieldRecoreder.add(r);
}

/*
 * Class:     logger_Record
 * Method:    addNewRecord
 * Signature: (JIII)V
 */
JNIEXPORT void JNICALL Java_logger_Record_addNewRecord
  (JNIEnv *jvmti_env, jclass klass, jlong addr, jint obj_typeid, jint line, jint class_name_id, jlong obj_size) {
    NewRecord<0> r;
    r.time = nanotime();
    r.addr = addr>>3;
    r.obj_typeid = obj_typeid;
    r.line = line;
    r.class_name_id = class_name_id;
    r.obj_size = obj_size;
    newRecoreder.add(r);
  }

/*
 * Class:     logger_Record
 * Method:    addAASTORERecord
 * Signature: (JIJII)V
 */
JNIEXPORT void JNICALL Java_logger_Record_addAASTORERecord
  (JNIEnv *jvmti_env, jclass klass, jlong holder_addr, jint index, jlong ref_addr, jint line, jint class_name_id) {
    AAStoreRecord<0> r;
    r.time = nanotime();
    r.holder_addr = holder_addr>>3;
    r.index = index;
    r.ref_addr = ref_addr>>3;
    r.line = line;
    r.class_name_id = class_name_id;
    aaStoreRecoreder.add(r);
  }

JNIEXPORT jlong JNICALL Java_logger_Record_getTid
  (JNIEnv *jvmti_env, jclass klass) {
    return syscall(SYS_gettid);
}

JNIEXPORT jlong JNICALL Java_logger_Record_getPid
  (JNIEnv *jvmti_env, jclass klass) {
    return getpid();
}

JNIEXPORT void JNICALL Java_logger_Record_addUseRecord
  (JNIEnv *jvmti_env, jclass klass, jlong addr, jint line, jint class_name_id) {
    UseRecord<0> r;
    r.time = nanotime();
    r.addr = addr>>3;
    r.line = line;
    r.class_name_id = class_name_id;
    useRecorder.add(r);
  }
// #define Java_logger_Record_addFieldValueRecord(type, jvmti_env, klass, addr, value, line, class_name_id, field_id, is_put) JNIEXPORT void JNICALL Java_logger_Record_addFieldValueRecord_##type \
//   (JNIEnv *jvmti_env, jclass klass, jlong addr, type value, jint line, jint class_name_id, jint field_id, jboolean is_put) { \
//     if(#type == "Jdouble") { \
//       if(is_put) { \
//         FieldValueRecord<0,1,1,1> r; \
//         r.time = nanotime(); \
//         r.addr = addr>>3; \
//         r.line = line; \
//         r.class_name_id = class_name_id; \
//         r.field_id = field_id; \
//         r.value = value; \
//         fieldValueRecord0111.add(r); \
//       } else { \
//         FieldValueRecord<0,0,1,1> r; \
//         r.time = nanotime(); \
//         r.addr = addr>>3; \
//         r.line = line; \
//         r.class_name_id = class_name_id; \
//         r.field_id = field_id; \
//         r.value = value; \
//         fieldValueRecord0011.add(r); \
//       } \
//     } else if(#type == "Jfloat") { \
//       if(is_put) { \
//         FieldValueRecord<0,1,0,1> r; \
//         r.time = nanotime(); \
//         r.addr = addr>>3; \
//         r.line = line; \
//         r.class_name_id = class_name_id; \
//         r.field_id = field_id; \
//         r.value = value; \
//         fieldValueRecord0101.add(r); \
//       } else { \
//         FieldValueRecord<0,0,0,1> r; \
//         r.time = nanotime(); \
//         r.addr = addr>>3; \
//         r.line = line; \
//         r.class_name_id = class_name_id; \
//         r.field_id = field_id; \
//         r.value = value; \
//         fieldValueRecord0001.add(r); \
//       } \
//     } else if(#type == "Jlong") { \
//       if(is_put) { \
//         FieldValueRecord<0,1,1,0> r; \
//         r.time = nanotime(); \
//         r.addr = addr>>3; \
//         r.line = line; \
//         r.class_name_id = class_name_id; \
//         r.field_id = field_id; \
//         r.value = value; \
//         fieldValueRecord0110.add(r); \
//       } else { \
//         FieldValueRecord<0,0,1,0> r; \
//         r.time = nanotime(); \
//         r.addr = addr>>3; \
//         r.line = line; \
//         r.class_name_id = class_name_id; \
//         r.field_id = field_id; \
//         r.value = value; \
//         fieldValueRecord0010.add(r); \
//       } \
//     } else { \
//       if(is_put) { \
//         FieldValueRecord<0,1,0,0> r; \
//         r.time = nanotime(); \
//         r.addr = addr>>3; \
//         r.line = line; \
//         r.class_name_id = class_name_id; \
//         r.field_id = field_id; \
//         r.value = value; \
//         fieldValueRecord0100.add(r); \
//       } else { \
//         FieldValueRecord<0,0,0,0> r; \
//         r.time = nanotime(); \
//         r.addr = addr>>3; \
//         r.line = line; \
//         r.class_name_id = class_name_id; \
//         r.field_id = field_id; \
//         r.value = value; \
//         fieldValueRecord0000.add(r); \
//       } \
//     } \
//   }

// Java_logger_Record_addFieldValueRecord(jint, jvmti_env, klass, addr, value, line, class_name_id, field_id, is_put)
// Java_logger_Record_addFieldValueRecord(jlong, jvmti_env, klass, addr, value, line, class_name_id, field_id, is_put)
// Java_logger_Record_addFieldValueRecord(jfloat, jvmti_env, klass, addr, value, line, class_name_id, field_id, is_put)
// Java_logger_Record_addFieldValueRecord(jdouble, jvmti_env, klass, addr, value, line, class_name_id, field_id, is_put)

JNIEXPORT void JNICALL Java_logger_Record_addFieldValueRecordJint
  (JNIEnv *jvmti_env, jclass klass, jlong addr, jint value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    if(is_put) { 
      FieldValueRecord<0,1,0,0> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = value; 
      fieldValueRecord0100.add(r); 
    } else { 
      FieldValueRecord<0,0,0,0> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = value; 
      fieldValueRecord0000.add(r); 
    }
  }

JNIEXPORT void JNICALL Java_logger_Record_addFieldValueRecordJlong
  (JNIEnv *jvmti_env, jclass klass, jlong addr, jlong value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    if(is_put) { 
      FieldValueRecord<0,1,1,0> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = value; 
      fieldValueRecord0110.add(r); 
    } else { 
      FieldValueRecord<0,0,1,0> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = value; 
      fieldValueRecord0010.add(r); 
    } 
  }
JNIEXPORT void JNICALL Java_logger_Record_addFieldValueRecordJfloat
  (JNIEnv *jvmti_env, jclass klass, jlong addr, jfloat value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    if(is_put) { 
      FieldValueRecord<0,1,0,1> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = *(uint32_t*) &value; 
      fieldValueRecord0101.add(r); 
    } else { 
      FieldValueRecord<0,0,0,1> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = *(uint32_t*) &value; 
      fieldValueRecord0001.add(r); 
    } 
  }
JNIEXPORT void JNICALL Java_logger_Record_addFieldValueRecordJdouble
  (JNIEnv *jvmti_env, jclass klass, jlong addr, jdouble value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    if(is_put) { 
      FieldValueRecord<0,1,1,1> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = *(uint64_t*) &value; 
      fieldValueRecord0111.add(r); 
    } else { 
      FieldValueRecord<0,0,1,1> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = *(uint64_t*) &value; 
      fieldValueRecord0011.add(r); 
    } 
  }
JNIEXPORT void JNICALL Java_logger_Record_addFieldValueRecordJstring
  (JNIEnv *jvmti_env, jclass klass, jlong addr, jstring value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    // TODO: memory leak, release string
    // printf("string = %s\n", jvmti_env->GetStringUTFChars(value, NULL));
    int string_id = stringMap.size();
    std::string tmp = jvmti_env->GetStringUTFChars(value, NULL);
    if(stringMap.find(tmp) == stringMap.end()) {
      std::string str(std::to_string(string_id)+","+tmp+"\n");
      if(is_put) {
        stringFieldValueRecord01.addString(str); 
      } else {
        stringFieldValueRecord00.addString(str); 
      }
      stringMap[tmp] = string_id;
    } else {
      string_id = stringMap[tmp];
    }
    if(is_put) { 
      StringFieldValueRecord<0,1> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.string_id = string_id; 
      stringFieldValueRecord01.add(r); 
    } else { 
      StringFieldValueRecord<0,0> r; 
      r.time = nanotime(); 
      r.addr = addr>>3; 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.string_id = string_id; 
      stringFieldValueRecord00.add(r); 
    } 
  }
JNIEXPORT void JNICALL Java_logger_Record_addStaticFieldValueRecordJint
  (JNIEnv *jvmti_env, jclass klass, jint value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    if(is_put) { 
      StaticFieldValueRecord<0,1,0,0> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = value; 
      staticFieldValueRecord0100.add(r); 
    } else { 
      StaticFieldValueRecord<0,0,0,0> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = value; 
      staticFieldValueRecord0000.add(r); 
    }
  }

JNIEXPORT void JNICALL Java_logger_Record_addStaticFieldValueRecordJlong
  (JNIEnv *jvmti_env, jclass klass, jlong value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    if(is_put) { 
      StaticFieldValueRecord<0,1,1,0> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = value; 
      staticFieldValueRecord0110.add(r); 
    } else { 
      StaticFieldValueRecord<0,0,1,0> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = value; 
      staticFieldValueRecord0010.add(r); 
    } 
  }

JNIEXPORT void JNICALL Java_logger_Record_addStaticFieldValueRecordJfloat
  (JNIEnv *jvmti_env, jclass klass, jfloat value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    if(is_put) { 
      StaticFieldValueRecord<0,1,0,1> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = *(uint32_t*) &value; 
      staticFieldValueRecord0101.add(r); 
    } else { 
      StaticFieldValueRecord<0,0,0,1> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = *(uint32_t*) &value; 
      staticFieldValueRecord0001.add(r); 
    } 
  }
JNIEXPORT void JNICALL Java_logger_Record_addStaticFieldValueRecordJdouble
  (JNIEnv *jvmti_env, jclass klass, jdouble value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    if(is_put) { 
      StaticFieldValueRecord<0,1,1,1> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = *(uint64_t*) &value; 
      staticFieldValueRecord0111.add(r); 
    } else { 
      StaticFieldValueRecord<0,0,1,1> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.value = *(uint64_t*) &value; 
      staticFieldValueRecord0011.add(r); 
    } 
  }
JNIEXPORT void JNICALL Java_logger_Record_addStaticFieldValueRecordJstring
  (JNIEnv *jvmti_env, jclass klass, jstring value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    // printf("string = %s\n", jvmti_env->GetStringUTFChars(value, NULL));
    int string_id = stringMap.size();
    std::string tmp = jvmti_env->GetStringUTFChars(value, NULL);
    if(stringMap.find(tmp) == stringMap.end()) {
      std::string str(std::to_string(string_id)+","+tmp+"\n");
      stringMap[tmp] = string_id;
      if(is_put) {
        staticStringFieldValueRecord01.addString(str); 
      } else {
        staticStringFieldValueRecord00.addString(str);
      }
    } else {
      string_id = stringMap[tmp];
    }
    if(is_put) { 
      StaticStringFieldValueRecord<0,1> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.string_id = string_id; 
      staticStringFieldValueRecord01.add(r); 
    } else { 
      StaticStringFieldValueRecord<0,0> r; 
      r.time = nanotime(); 
      r.line = line; 
      r.class_name_id = class_name_id; 
      r.field_id = field_id; 
      r.string_id = string_id; 
      staticStringFieldValueRecord00.add(r); 
    } 
  }

JNIEXPORT jobject JNICALL Java_logger_Record_addContainerRecordList15
  (JNIEnv *jvmti_env, jclass klass, jobject retobj, jlong ref_addr, jlong holder_addr, jint idx_or_succ, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = ref_addr>>3;
    r.idx_or_succ = idx_or_succ;
    r.br.size = size;
    listAndSetRecord.add(r);
    return retobj;
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerRecordList6
  (JNIEnv *jvmti_env, jclass klass, jlong holder_addr, jint idx, jlong ref_addr, jint size, jint cid, jint copid) {
    if(!enable) return ;
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = ref_addr>>3;
    r.idx_or_succ = idx;
    r.br.size = size;
    listAndSetRecord.add(r);
    return ;
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerRecordList4
  (JNIEnv *jvmti_env, jclass klass, jlong holder_addr, jint size, jint cid, jint copid) {
    if(!enable) return ;
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = 0;
    r.idx_or_succ = 0;
    r.br.size = size;
    listAndSetRecord.add(r);
    return ;
  }

JNIEXPORT jint JNICALL Java_logger_Record_addContainerRecordListI5
  (JNIEnv *jvmti_env, jclass klass, jint idx, jlong holder_addr, jlong ref_addr, jint size, jint cid, jint copid) {
    if(!enable) return idx;
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = ref_addr>>3;
    r.idx_or_succ = idx;
    r.br.size = size;
    listAndSetRecord.add(r);
    return idx;
  }

JNIEXPORT jobject JNICALL Java_logger_Record_addContainerRecordList14
  (JNIEnv *jvmti_env, jclass klass, jobject retobj, jlong ref_addr, jlong holder_addr, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = ref_addr>>3;
    r.idx_or_succ = 0; // empty
    r.br.size = size;
    listAndSetRecord.add(r);
    return retobj;
  }

JNIEXPORT jboolean JNICALL Java_logger_Record_addContainerRecordList5
  (JNIEnv *jvmti_env, jclass klass, jboolean retobj, jlong holder_addr, jlong ref_addr, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = ref_addr>>3;
    r.idx_or_succ = (retobj == 1);
    r.br.size = size;
    listAndSetRecord.add(r);
    return retobj;
  }

JNIEXPORT jobject JNICALL Java_logger_Record_addContainerRecordMap15
  (JNIEnv *jvmti_env, jclass klass, jobject retobj, jlong ref_addr, jlong holder_addr, jlong key_addr, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    MapRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = ref_addr>>3;
    r.key_addr = key_addr>>3;
    r.value_addr = ref_addr>>3; // redundant ref_addr
    r.br.size = size;
    mapRecord.add(r);
    return retobj;
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerRecordMap4
  (JNIEnv *jvmti_env, jclass klass, jlong holder_addr, jint size, jint cid, jint copid) {
    if(!enable) return;
    MapRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = 0;
    r.key_addr = 0;
    r.value_addr = 0;
    r.br.size = size;
    mapRecord.add(r);
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerRecordMap5
  (JNIEnv *jvmti_env, jclass klass, jlong holder_addr, jlong map_addr, jint size, jint cid, jint copid) {
    if(!enable) return;
    MapRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = map_addr>>3;
    r.key_addr = 0;
    r.value_addr = 0;
    r.br.size = size;
    mapRecord.add(r);
  }

JNIEXPORT jobject JNICALL Java_logger_Record_addContainerRecordMap16
  (JNIEnv *jvmti_env, jclass klass, jobject retobj, jlong ref_addr, jlong holder_addr, jlong key_addr, jlong value_addr, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    MapRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_addr>>3;
    r.br.ref_addr = ref_addr>>3;
    r.key_addr = key_addr>>3;
    r.value_addr = value_addr>>3;
    r.br.size = size;
    mapRecord.add(r);
    return retobj;
  }

JNIEXPORT void JNICALL Java_logger_Record_decreaseContainerFilterFlag
  (JNIEnv *jvmti_env, jclass klass) {
    if(is_right_after) {
      is_right_after = false;
    } 
  }

JNIEXPORT void JNICALL Java_logger_Record_unsetIsRightAfter
  (JNIEnv *jvmti_env, jclass klass) {
    is_right_after = false;
  }

JNIEXPORT void JNICALL Java_logger_Record_enableRecord
  (JNIEnv *jvmti_env, jclass klass) {
    enable = true;
  }

JNIEXPORT void JNICALL Java_logger_Record_disableRecord
  (JNIEnv *jvmti_env, jclass klass) {
    enable = false;
  }

JNIEXPORT void JNICALL Java_logger_Record_enablePrintNew
  (JNIEnv *jvmti_env, jclass klass) {
    enablePrintNEW = true;
  }

JNIEXPORT void JNICALL Java_logger_Record_disablePrintNew
  (JNIEnv *jvmti_env, jclass klass) {
    enablePrintNEW = false;
  }

JNIEXPORT jboolean JNICALL Java_logger_Record_getPrintNew
  (JNIEnv *jvmti_env, jclass klass) {
    return enablePrintNEW ? JNI_TRUE : JNI_FALSE;
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerLineInfoRecord
  (JNIEnv *jvmti_env, jclass klass, jint line, jint class_name_id) {
    if(!enable) return;
    ContainerLineInfoRecord r; 
    r.time = nanotime(); 
    r.line = line; 
    r.class_name_id = class_name_id;
    containerLineInfoRecorder.add(r);
    is_right_after = true;
  }

JNIEXPORT void JNICALL Java_logger_Record_addFreeRecord
  (JNIEnv *jvmti_env, jclass klass, jlong addr, jlong obj_size) {
    FreeRecord<0> r;
    r.time = nanotime();
    r.addr = addr>>3;
    r.obj_size = obj_size;
    freeRecorder.add(r);
  }
JNIEXPORT void JNICALL Java_logger_Record_doSomething
  (JNIEnv *jvmti_env, jclass klass, jlong addr) {
      uint32_t* raw_addr = (uint32_t*)addr;
      printf("native: %u %u %u %u %u %u\n", raw_addr[0], raw_addr[1], raw_addr[2], raw_addr[3], raw_addr[4], raw_addr[5]);
      raw_addr[3] += 1;
  }
JNIEXPORT jlong JNICALL Java_logger_Record_allocateArgsBuffer
  (JNIEnv *jvmti_env, jclass klass) {

    if (!args_buffer) {
      args_buffer = new int[32];
      memset(args_buffer, 0, 32*sizeof(int));
    }
    return (jlong)args_buffer;
  }
JNIEXPORT void JNICALL Java_logger_Record_printArgs
  (JNIEnv *, jclass, jint pos, jint type)
  {

    if (type == 0) {
      // int
      // spdlog::info("args[{}] for type {}: {}", pos, "int", load_big_endian<int>(args_buffer+pos));
      spdlog::info("args[{}] for type {}: {}", pos, "int", args_buffer[pos]);
    }
    else if (type == 1) {
      // double
      spdlog::info("args[{}] for type {}: {}", pos, "double", *(double*)(args_buffer+pos));
    } else if (type == 2) {
      // float 
      spdlog::info("args[{}] for type {}: {}", pos, "float", *(float*)(args_buffer+pos));
    } else if (type == 3) {
      // long 
      spdlog::info("args[{}] for type {}: {}", pos, "long", *(long*)(args_buffer+pos));
    }
  }
JNIEXPORT void JNICALL Java_logger_Record_putInt
  (JNIEnv *, jclass, jlong addr, jint val) {
    *(jint*)addr = val;
  }
JNIEXPORT void JNICALL Java_logger_Record_putDouble
  (JNIEnv *, jclass, jlong addr, jdouble val) {
    *(jdouble*)addr = val;
  }
JNIEXPORT void JNICALL Java_logger_Record_putFloat
  (JNIEnv *, jclass, jlong addr, jfloat val) {
    *(jfloat*)addr = val;
  }
JNIEXPORT void JNICALL Java_logger_Record_putLong
  (JNIEnv *, jclass, jlong addr, jlong val) {
    *(jlong*)addr = val;
  }