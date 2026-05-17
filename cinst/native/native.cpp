#include "jni.h"
#include "logger_Record.h"
#include "recorder.hpp"
#include <atomic>
#include <sys/syscall.h>
#include <unistd.h>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include "jvmti.h"
#include <stdlib.h>

constexpr long NOT_SAMPLED_MASK = (1L<<63);

int sampling_freq =  getenv("JLITE_SAMPLING_FREQ") ? atoi(getenv("JLITE_SAMPLING_FREQ")) : 1000;



thread_local int* args_buffer = nullptr;
thread_local jvmtiEnv* local_jvmti_env = NULL;

constexpr int LIMIT=(1<<12);
thread_local bool enablePrintNEW = true; // enable printNEW or not?

// TODO: allcote the _buffer for specific Recorder when necessary!
thread_local std::unordered_map<std::string, int> stringMap;
thread_local Recorder<PutFieldRecord<0>> putFieldRecoreder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<NewRecord<0>> newRecoreder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<ConstructRecord<0>> constructRecoreder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<AAStoreRecord<0>> aaStoreRecoreder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<UseRecord<0>> useRecorder(LIMIT, syscall(SYS_gettid), getpid());
thread_local Recorder<FreeRecord<0>> freeRecorder(LIMIT, syscall(SYS_gettid), getpid());

// for container mode 
// for recursion: e.g. HashSet will call HashMap op
thread_local bool is_right_after = false;
thread_local bool enable = true; // enable record or not?
thread_local bool is_sampled_container = true;
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

constexpr size_t JVMTI_BUCKET_SIZE = 1024;
static jvmtiEnv* jvmti_env[JVMTI_BUCKET_SIZE+1];

/*
 * Class:     logger_Record
 * Method:    addPutFieldRecord
 * Signature: (JJII)V
 */
JNIEXPORT void JNICALL Java_logger_Record_addPutFieldRecord
  (JNIEnv *jni_env, jclass klass, jlong holder_addr, jlong ref_addr, jint line, jint class_name_id, int field_id) {
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
  (JNIEnv *jni_env, jclass klass, jlong addr, jint obj_typeid, jint line, jint class_name_id, jlong obj_size) {
    NewRecord<0> r;
    r.time = nanotime();
    r.addr = addr>>3;
    r.obj_typeid = obj_typeid;
    r.line = line;
    r.class_name_id = class_name_id;
    r.obj_size = obj_size;
    newRecoreder.add(r);
  }

JNIEXPORT void JNICALL Java_logger_Record_addConstructRecord
  (JNIEnv *jni_env, jclass klass, jlong addr, jint obj_typeid, jint line, jint class_name_id) {
    ConstructRecord<0> r;
    r.time = nanotime();
    r.addr = addr>>3;
    r.obj_typeid = obj_typeid;
    r.line = line;
    r.class_name_id = class_name_id;
    constructRecoreder.add(r);
  }
/*
 * Class:     logger_Record
 * Method:    addAASTORERecord
 * Signature: (JIJII)V
 */
JNIEXPORT void JNICALL Java_logger_Record_addAASTORERecord
  (JNIEnv *jni_env, jclass klass, jlong holder_addr, jint index, jlong ref_addr, jint line, jint class_name_id) {
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
  (JNIEnv *jni_env, jclass klass) {
    return syscall(SYS_gettid);
}

JNIEXPORT jlong JNICALL Java_logger_Record_getPid
  (JNIEnv *jni_env, jclass klass) {
    return getpid();
}

JNIEXPORT void JNICALL Java_logger_Record_addUseRecord
  (JNIEnv *jni_env, jclass klass, jint type, jlong addr, jint line, jint class_name_id) {
    UseRecord<0> r;
    r.type = type;
    r.time = nanotime();
    r.addr = addr>>3;
    r.line = line;
    r.class_name_id = class_name_id;
    useRecorder.add(r);
  }
// #define Java_logger_Record_addFieldValueRecord(type, jni_env, klass, addr, value, line, class_name_id, field_id, is_put) JNIEXPORT void JNICALL Java_logger_Record_addFieldValueRecord_##type \
//   (JNIEnv *jni_env, jclass klass, jlong addr, type value, jint line, jint class_name_id, jint field_id, jboolean is_put) { \
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

// Java_logger_Record_addFieldValueRecord(jint, jni_env, klass, addr, value, line, class_name_id, field_id, is_put)
// Java_logger_Record_addFieldValueRecord(jlong, jni_env, klass, addr, value, line, class_name_id, field_id, is_put)
// Java_logger_Record_addFieldValueRecord(jfloat, jni_env, klass, addr, value, line, class_name_id, field_id, is_put)
// Java_logger_Record_addFieldValueRecord(jdouble, jni_env, klass, addr, value, line, class_name_id, field_id, is_put)

JNIEXPORT void JNICALL Java_logger_Record_addFieldValueRecordJint
  (JNIEnv *jni_env, jclass klass, jlong addr, jint value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
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
  (JNIEnv *jni_env, jclass klass, jlong addr, jlong value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
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
  (JNIEnv *jni_env, jclass klass, jlong addr, jfloat value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
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
  (JNIEnv *jni_env, jclass klass, jlong addr, jdouble value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
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
  (JNIEnv *jni_env, jclass klass, jlong addr, jstring value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    // TODO: memory leak, release string
    // printf("string = %s\n", jni_env->GetStringUTFChars(value, NULL));
    int string_id = stringMap.size();
    std::string tmp = jni_env->GetStringUTFChars(value, NULL);
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
  (JNIEnv *jni_env, jclass klass, jint value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
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
  (JNIEnv *jni_env, jclass klass, jlong value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
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
  (JNIEnv *jni_env, jclass klass, jfloat value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
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
  (JNIEnv *jni_env, jclass klass, jdouble value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
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
  (JNIEnv *jni_env, jclass klass, jstring value, jint line, jint class_name_id, jint field_id, jboolean is_put) {
    // printf("string = %s\n", jni_env->GetStringUTFChars(value, NULL));
    int string_id = stringMap.size();
    std::string tmp = jni_env->GetStringUTFChars(value, NULL);
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

bool is_sampled_object(long tag);
void get_tag(jobject obj, long *tag);
void init_jvmti(JNIEnv*jni_env);
JNIEXPORT jobject JNICALL Java_logger_Record_addContainerRecordList15
  (JNIEnv *jni_env, jclass klass, jobject retobj, jobject holder, jint idx_or_succ, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    long ref_tag, holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return retobj;
    }
    is_sampled_container = true;
    get_tag(retobj, &ref_tag);
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = ref_tag;
    r.idx_or_succ = idx_or_succ;
    r.br.size = size;
    listAndSetRecord.add(r);
    return retobj;
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerRecordList6
  (JNIEnv *jni_env, jclass klass, jobject holder, jint idx, jobject ref, jint size, jint cid, jint copid) {
    if(!enable) return;
    long ref_tag, holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return;
    }
    is_sampled_container = true;
    get_tag(ref, &ref_tag);
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = ref_tag;
    r.idx_or_succ = idx;
    r.br.size = size;
    listAndSetRecord.add(r);
    return ;
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerRecordList4
  (JNIEnv *jni_env, jclass klass, jobject holder, jint size, jint cid, jint copid) {
    if(!enable) return;
    long holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return;
    }
    is_sampled_container = true;
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = 0;
    r.idx_or_succ = 0;
    r.br.size = size;
    listAndSetRecord.add(r);
    return ;
  }

JNIEXPORT jint JNICALL Java_logger_Record_addContainerRecordListI5
  (JNIEnv *jni_env, jclass klass, jint idx, jobject holder, jobject ref, jint size, jint cid, jint copid) {
    if(!enable) return idx;
    long ref_tag, holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return idx;
    }
    is_sampled_container = true;
    get_tag(ref, &ref_tag);
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = ref_tag;
    r.idx_or_succ = idx;
    r.br.size = size;
    listAndSetRecord.add(r);
    return idx;
  }

JNIEXPORT jobject JNICALL Java_logger_Record_addContainerRecordList14
  (JNIEnv *jni_env, jclass klass, jobject retobj, jobject holder, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    long ref_tag, holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return retobj;
    }
    is_sampled_container = true;
    get_tag(retobj, &ref_tag);
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = ref_tag;
    r.idx_or_succ = 0; // empty
    r.br.size = size;
    listAndSetRecord.add(r);
    return retobj;
  }

JNIEXPORT jboolean JNICALL Java_logger_Record_addContainerRecordList5
  (JNIEnv *jni_env, jclass klass, jboolean retobj, jobject holder, jobject ref, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    long ref_tag, holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return retobj;
    }
    is_sampled_container = true;
    get_tag(ref, &ref_tag);
    ListAndSetRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = ref_tag;
    r.idx_or_succ = (retobj == 1);
    r.br.size = size;
    listAndSetRecord.add(r);
    return retobj;
  }

JNIEXPORT jobject JNICALL Java_logger_Record_addContainerRecordMap15
  (JNIEnv *jni_env, jclass klass, jobject retobj, jobject holder, jobject key, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    long ref_tag, key_tag, holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return retobj;
    }
    is_sampled_container = true;
    get_tag(retobj, &ref_tag);
    get_tag(key, &key_tag);
    MapRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = ref_tag;
    r.key_addr = key_tag;
    r.value_addr = ref_tag; // redundant ref_addr
    r.br.size = size;
    mapRecord.add(r);
    return retobj;
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerRecordMap4
  (JNIEnv *jni_env, jclass klass, jobject holder, jint size, jint cid, jint copid) {
    if(!enable) return;
    long holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return;
    }
    is_sampled_container = true;
    MapRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = 0;
    r.key_addr = 0;
    r.value_addr = 0;
    r.br.size = size;
    mapRecord.add(r);
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerRecordMap5
  (JNIEnv *jni_env, jclass klass, jobject holder, jobject map, jint size, jint cid, jint copid) {
    if(!enable) return;
    long map_tag, holder_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return;
    }
    is_sampled_container = true;
    get_tag(map, &map_tag);
    MapRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = map_tag;
    r.key_addr = 0;
    r.value_addr = 0;
    r.br.size = size;
    mapRecord.add(r);
  }

JNIEXPORT jobject JNICALL Java_logger_Record_addContainerRecordMap16
  (JNIEnv *jni_env, jclass klass, jobject retobj, jobject holder, jobject key, jobject value, jint size, jint cid, jint copid) {
    if(!enable) return retobj;
    long ref_tag, holder_tag, key_tag, value_tag;
    init_jvmti(jni_env);
    // holder means this container who calls the method
    get_tag(holder, &holder_tag);
    if (!is_sampled_object(holder_tag))
    {
      is_sampled_container = false;
      return retobj;
    }
    is_sampled_container = true;
    get_tag(retobj, &ref_tag);
    get_tag(key, &key_tag);
    get_tag(value, &value_tag);
    MapRecord<0> r; 
    r.br.time = nanotime(); 
    r.br.cid = cid; 
    r.br.copid = copid; 
    r.br.holder_addr = holder_tag;
    r.br.ref_addr = ref_tag;
    r.key_addr = key_tag;
    r.value_addr = value_tag;
    r.br.size = size;
    mapRecord.add(r);
    return retobj;
  }

JNIEXPORT void JNICALL Java_logger_Record_decreaseContainerFilterFlag
  (JNIEnv *jni_env, jclass klass) {
    if(is_right_after) {
      is_right_after = false;
    } 
  }

JNIEXPORT void JNICALL Java_logger_Record_unsetIsRightAfter
  (JNIEnv *jni_env, jclass klass) {
    is_right_after = false;
  }

JNIEXPORT void JNICALL Java_logger_Record_enableRecord
  (JNIEnv *jni_env, jclass klass) {
    enable = true;
  }

JNIEXPORT void JNICALL Java_logger_Record_disableRecord
  (JNIEnv *jni_env, jclass klass) {
    enable = false;
  }

JNIEXPORT void JNICALL Java_logger_Record_enablePrintNew
  (JNIEnv *jni_env, jclass klass) {
    enablePrintNEW = true;
  }

JNIEXPORT void JNICALL Java_logger_Record_disablePrintNew
  (JNIEnv *jni_env, jclass klass) {
    enablePrintNEW = false;
  }

JNIEXPORT jboolean JNICALL Java_logger_Record_getPrintNew
  (JNIEnv *jni_env, jclass klass) {
    return enablePrintNEW ? JNI_TRUE : JNI_FALSE;
  }

JNIEXPORT void JNICALL Java_logger_Record_addContainerLineInfoRecord
  (JNIEnv *jni_env, jclass klass, jint line, jint class_name_id) {
  // (JNIEnv *jni_env, jclass klass, jobject holder, jint line, jint class_name_id) {
    if(!enable || !is_sampled_container) return;
    ContainerLineInfoRecord r; 
    r.time = nanotime(); 
    r.line = line; 
    r.class_name_id = class_name_id;
    containerLineInfoRecorder.add(r);
    is_right_after = true;
  }

JNIEXPORT void JNICALL Java_logger_Record_addFreeRecord
  (JNIEnv *jni_env, jclass klass, jlong addr, jlong obj_size) {
    FreeRecord<0> r;
    r.time = nanotime();
    r.addr = addr>>3;
    r.obj_size = obj_size;
    freeRecorder.add(r);
  }
JNIEXPORT void JNICALL Java_logger_Record_doSomething
  (JNIEnv *jni_env, jclass klass, jlong addr) {
      uint32_t* raw_addr = (uint32_t*)addr;
      printf("native: %u %u %u %u %u %u\n", raw_addr[0], raw_addr[1], raw_addr[2], raw_addr[3], raw_addr[4], raw_addr[5]);
      raw_addr[3] += 1;
  }
JNIEXPORT jlong JNICALL Java_logger_Record_allocateArgsBuffer
  (JNIEnv *jni_env, jclass klass) {

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
void init_jvmti(JNIEnv*jni_env)
{
    if (!jvmti_env[0])
    {
      for (int i = 0; i < JVMTI_BUCKET_SIZE+1; ++i)
      {
        JavaVM* vm;
        (*jni_env).GetJavaVM(&vm);
        auto code = vm->GetEnv((void**)&jvmti_env[i], JVMTI_VERSION_1_0);
        if (code != JVMTI_ERROR_NONE)
        {
          spdlog::critical("Failed to get jvmti env: {}", code);
        }
        jvmtiCapabilities caps;
        // 分配内存
        memset(&caps, 0, sizeof(caps));
        caps.can_tag_objects = 1;
        jvmti_env[i]->AddCapabilities(&caps);

      }


    }

    if (!local_jvmti_env)
    {
      JavaVM* vm;
      (*jni_env).GetJavaVM(&vm);
      auto code = vm->GetEnv((void**)&local_jvmti_env, JVMTI_VERSION_1_0);
      if (code != JVMTI_ERROR_NONE)
      {
        spdlog::critical("Failed to get jvmti env: {}", code);
      }
      jvmtiCapabilities caps;
      // 分配内存
      memset(&caps, 0, sizeof(caps));
      caps.can_tag_objects = 1;
      local_jvmti_env->AddCapabilities(&caps);

    }
}

bool new_called = false;
bool jvmti_valid()
{
  return jvmti_env[0] != NULL;
}


// void get_tag(jobject obj, long *tag, int hashcode)
// {
//   jvmti_env[hashcode%JVMTI_BUCKET_SIZE]->GetTag(obj, tag);
// }
// void get_tag(jobject obj, long *tag)
// {
//   int hashcode;
//   jvmti_env[JVMTI_BUCKET_SIZE]->GetObjectHashCode(obj, &hashcode);
//   jvmti_env[hashcode%JVMTI_BUCKET_SIZE]->GetTag(obj, tag);
// }
// void set_tag(jobject obj, long tag)
// {
//   int hashcode;
//   jvmti_env[JVMTI_BUCKET_SIZE]->GetObjectHashCode(obj, &hashcode);
//   jvmti_env[hashcode%JVMTI_BUCKET_SIZE]->SetTag(obj, tag);
// }
void get_tag(jobject obj, long *tag)
{
  long local_tag;
  local_jvmti_env->GetTag(obj, &local_tag);
  if (local_tag != 0)
  {
    *tag = local_tag;
    return;
  }

  jvmti_env[0]->GetTag(obj, &local_tag);
  if (local_tag != 0)
  {
    *tag = local_tag;
    local_jvmti_env->SetTag(obj, local_tag);
    return;
  }
  else 
  {
    *tag = -1L;
    local_jvmti_env->SetTag(obj, -1L);
  }
}

void set_tag(jobject obj, long tag)
{
  local_jvmti_env->SetTag(obj, tag);
  jvmti_env[0]->SetTag(obj, tag);
}
std::atomic<long> global_tag {10};

bool is_sampled_object(long tag)
{
  return tag != 0 && !(tag & (NOT_SAMPLED_MASK)) && (tag != -1L);
}
JNIEXPORT void JNICALL Java_logger_Record_addTagNewRecord
  (JNIEnv *jni_env, jclass klass, jobject obj, jint obj_typeid, jint line, jint class_name_id, jlong obj_size) {
    if (1./sampling_freq < rand()/(RAND_MAX+1.)) {
      return;
    }
    NewRecord<0> r;
    r.time = nanotime();
    long tag = global_tag++;
    r.addr = tag;
    r.obj_typeid = obj_typeid;
    r.line = line;
    r.class_name_id = class_name_id;
    r.obj_size = obj_size;
    newRecoreder.add(r);
    init_jvmti(jni_env);
    set_tag(obj, tag);
    new_called = true;
  }
JNIEXPORT void JNICALL Java_logger_Record_addTagConstructRecord
  (JNIEnv *jni_env, jclass klass, jobject obj, jint obj_typeid, jint line, jint class_name_id) {
    ConstructRecord<0> r;
    long tag;
    init_jvmti(jni_env);
    get_tag(obj, &tag);
    if (!is_sampled_object(tag))
    {
      return;
    }
    r.time = nanotime();
    r.addr = tag;
    r.obj_typeid = obj_typeid;
    r.line = line;
    r.class_name_id = class_name_id;
    constructRecoreder.add(r);
  }

JNIEXPORT void JNICALL Java_logger_Record_addTagPutFieldRecord
  (JNIEnv *jni_env, jclass klass, jobject holder, jobject ref, jint line, jint class_name_id, int field_id) {
    PutFieldRecord<0> r;
    r.time = nanotime();
    long tag;
    init_jvmti(jni_env);
    get_tag(ref, &tag);
    if (!is_sampled_object(tag))
    {
      return;
    }
    r.ref_addr = tag;
    get_tag(holder, &tag);
    if (tag == 0)
    {
      tag = global_tag++;
      set_tag(holder, tag | NOT_SAMPLED_MASK);
    }
    r.holder_addr = tag;
    r.field_id = field_id;
    r.line = line;
    r.class_name_id = class_name_id;
    putFieldRecoreder.add(r);
}
JNIEXPORT void JNICALL Java_logger_Record_addTagAASTORERecord
  (JNIEnv *jni_env, jclass klass, jobject holder, jint index, jobject ref, jint line, jint class_name_id) {
    AAStoreRecord<0> r;
    long tag;
    init_jvmti(jni_env);
    get_tag(ref, &tag);
    if (!is_sampled_object(tag))
    {
      return;
    }
    r.time = nanotime();
    r.ref_addr = tag;
    r.index = index;
    get_tag(holder, &tag);
    if (tag == 0)
    {
      tag = global_tag++;
      set_tag(holder, tag | NOT_SAMPLED_MASK);
    }
    r.holder_addr = tag;
    r.line = line;
    r.class_name_id = class_name_id;
    aaStoreRecoreder.add(r);
  }
JNIEXPORT void JNICALL Java_logger_Record_addTagUseRecord
  (JNIEnv *jni_env, jclass klass, jint type, jobject obj, jint line, jint class_name_id) {
    if (!new_called)
    {
      return;
    }
    if (!jvmti_valid())
    {
      return;
    }
    // spdlog::info("addTagUseRecord: {}, {}, {}", (long)obj, *(long*)obj, jni_env->IsSameObject(obj, NULL));
    // spdlog::info("addTagUseRecord: {}", (long)obj);
    // if (*(long*)obj % 8 != 0 || *(long*)obj < 1000)
    // {
    //   return;
    // }
    init_jvmti(jni_env);
    long tag;
    get_tag(obj, &tag);
    if (!is_sampled_object(tag))
    {
      return;
    }
    UseRecord<0> r;
    r.type = type;
    r.time = nanotime();
    r.addr = tag;
    r.line = line;
    r.class_name_id = class_name_id;
    useRecorder.add(r);
  }