#include <iostream>
#include <unordered_set>
#include <cstring>
#include "agent.h"
#include <string>
#include <assert.h>
#include "utils.h"
#include "spdlog/spdlog.h"
#include "client.h"
#include "logger_Record.h"
using std::string;
using std::unordered_set;
using std::ostream;
using std::endl;

extern void set_gc_start(bool);
bool vm_init = false;

// 需要过滤的包名
static char *filter_package = NULL;

static jvmtiEnv* jvmti_env;

template <class T>
class JvmtiDeallocator {
 public:
  JvmtiDeallocator() {
    elem_ = NULL;
  }

  ~JvmtiDeallocator() {
    jvmti_env->Deallocate(reinterpret_cast<unsigned char*>(elem_));
  }

  T* get_addr() {
    return &elem_;
  }

  T get() {
    return elem_;
  }

 private:
  T elem_;
};

static void GetJMethodIDs(jclass klass) {
  jint method_count = 0;
  JvmtiDeallocator<jmethodID*> methods;
  jvmtiError err = jvmti_env->GetClassMethods(klass, &method_count, methods.get_addr());

  // If ever the GetClassMethods fails, just ignore it, it was worth a try.
  if (err != JVMTI_ERROR_NONE) {
    fprintf(stderr, "GetJMethodIDs: Error in GetClassMethods: %d\n", err);
  }
}

JNIEXPORT void JNICALL Java_logger_Record_printCallTrace
  (JNIEnv *env, jclass klass) {
    ASGCTType agct = reinterpret_cast<ASGCTType>(dlsym(RTLD_DEFAULT, "AsyncGetCallTrace"));

    const int MAX_DEPTH = 16;
    ASGCT_CallTrace trace;
    ASGCT_CallFrame frames[MAX_DEPTH];
    trace.frames = frames;
    trace.env_id = env;
    trace.num_frames = 0;

    if (agct == NULL) {
        fprintf(stderr, "AsyncGetCallTrace not found.\n");
    }

    agct(&trace, MAX_DEPTH, NULL);

    // For now, just check that the first frame is (-3, checkAsyncGetCallTraceCall).
    if (trace.num_frames <= 0) {
        fprintf(stderr, "The num_frames must be positive: %d\n", trace.num_frames);
    }
    fprintf(stderr, "num_frames=%d\n", trace.num_frames);
    for(int i = 0; i < trace.num_frames; i++) {
    char* name = NULL;
    if (trace.frames[i].method_id == NULL) {
        fprintf(stderr, "Number %d frame method_id is NULL\n", i);
        continue;
    }

    jclass declaring_class;
    jvmti_env->GetMethodDeclaringClass(trace.frames[i].method_id, &declaring_class);

    JvmtiDeallocator<char*> declaringClassName;
    jvmti_env->GetClassSignature(declaring_class, declaringClassName.get_addr(), NULL);
    fprintf(stderr, "classname = %s, ", declaringClassName.get());

    jvmtiError err = jvmti_env->GetMethodName(trace.frames[i].method_id, &name, NULL, NULL);
    if (err != JVMTI_ERROR_NONE) {
        fprintf(stderr, "checkAsyncGetCallTrace: Error in GetMethodName: %d\n", err);
    }

    if (name == NULL) {
        fprintf(stderr, "Name is NULL\n");
    }

    fprintf(stderr, "name = %s\n", name);
    }
    return;
  }


void check_jlocation_format(jvmtiEnv *jvmti_env)
{
    jvmtiJlocationFormat jf;
    jvmti_env->GetJLocationFormat(&jf);
    assert(jf == JVMTI_JLOCATION_JVMBCI && "Unsupported JVMTI JLocation format");
}
MODE mode = INVALID;
unordered_set<string> candidates;
void parse_args(const string& args) {
    if (args == "base") {
        mode = BASEMODE;
    } else if (args == "use" || args == "value") {
        if(args == "use") mode = USEMODE;
        else mode = VALUEMODE;
        FILE* f = fopen("candidate.txt", "r");
        if (f == NULL) {
            spdlog::error("use/value mode but not candidate.txt not found");
            throw nullptr;
        }
        while (true) {
            char buf[256];
            fgets(buf, 256, f);
            if (feof(f)) {
                break;
            }
            buf[strlen(buf)-1] = 0;
            spdlog::debug("candidate class: {}", buf);
            candidates.insert(buf);

        }
        fclose(f);
        spdlog::debug("match: {}", candidates.find("scala/Tuple2") != candidates.end());
    } else if(args == "container") mode = CONTAINERMODE;
}

// JVM 通过回调该方法启动 Agent
JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
    //fprintf(stdout, "开始加载自定义 agent\n");
    // 将 option 参数设为要过滤的包名
    //filter_package = options;
    // spdlog::set_level(spdlog::level::debug);
    spdlog::debug("agent startup");

    string args = options ? options : "base";
    parse_args(options);

    spdlog::debug("send hello to server");
    {
        ASMClient client;
        client.send_hello();

    }


    // jvmtiEnv *jvmti_env = NULL;

    jvm->GetEnv((void **)(&jvmti_env), JVMTI_VERSION_1_1);

    check_jlocation_format(jvmti_env);
    // 配置该 jvmti 要开启的功能
    jvmtiCapabilities caps;
    // 分配内存
    memset(&caps, 0, sizeof(caps));
    // 生成方法进入退出事件
    caps.can_generate_method_entry_events = 1;
    //caps.can_generate_method_exit_events = 1;
    //caps.can_generate_field_modification_events = 1;
    caps.can_generate_garbage_collection_events = 1;
    caps.can_retransform_classes = 1;
    caps.can_retransform_any_class = 1;
    caps.can_redefine_any_class = 1;
    caps.can_redefine_classes = 1;

    // 启用 jvmti 功能
    jvmti_env->AddCapabilities(&caps);

    // 定义回调方法， 配置事件对应的回调方法
    jvmtiEventCallbacks callBacks;
    memset(&callBacks, 0, sizeof(callBacks));
    callBacks.VMDeath = &VMDeath;
    callBacks.MethodEntry = &MethodEntry;
    //callBacks.MethodExit = &MethodExit;
    //callBacks.FieldModification = &FieldModification;
    callBacks.ClassPrepare = &ClassPrepare;
    callBacks.ClassLoad = &ClassLoad;
    callBacks.GarbageCollectionStart = &GarbageCollectionStart;
    callBacks.GarbageCollectionFinish = &GarbageCollectionFinish;
    callBacks.ClassFileLoadHook = &ClassFileLoadHook;


    // 设置 jvmti 事件回调
    jvmti_env->SetEventCallbacks(&callBacks, sizeof(callBacks));

    // JVM 退出事件监听
    jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, NULL);
    jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
    //jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, NULL);
    //jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_FIELD_MODIFICATION, NULL);
    jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, NULL);
    jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_LOAD, NULL);
    jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, NULL);
    jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL);
    jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);

    //fprintf(stdout, "成功加载自定义 agent\n");
    return JNI_OK;
}

// 进入方法是时的回调方法
static void JNICALL
MethodEntry(jvmtiEnv *jvmti_env,
            JNIEnv *jni_env,
            jthread thread,
            jmethodID method)
{
    // 获取执行类
    jclass clazz;
    jvmti_env->GetMethodDeclaringClass(method, &clazz);

    // 获执行类的签名
    char *class_signature_ptr;
    char* method_name;
    jvmti_env->GetClassSignature(clazz, &class_signature_ptr, NULL);
    jvmti_env->GetMethodName(method, &method_name, nullptr, nullptr);
    spdlog::debug("class signature: {}:{}", class_signature_ptr, method_name);


    // if (strcmp("Ljava.util.HashMap;", class_signature_ptr) == 0 || 
    //     strcmp("Ljava.util.HashSet;", class_signature_ptr) == 0 ||
    //     strcmp("Ljava.util.TreeMap;", class_signature_ptr) == 0 ||
    //     strcmp("Ljava.util.TreeSet;", class_signature_ptr) == 0 ||
    //     strcmp("Ljava.util.LinkedList;", class_signature_ptr) == 0 ||
    //     strcmp("Ljava.util.ArrayList;", class_signature_ptr) == 0) {
    //     jclass logger_record = jni_env->FindClass("logger/Record");
    //     jmethodID logger_record_init = jni_env->GetStaticMethodID(logger_record, "init", "()Z");
    //     bool r = jni_env->CallStaticBooleanMethod(logger_record, logger_record_init);
    //     if(r) spdlog::debug("logger/Record init successfully!");
    // }
    // if (strcmp("Ljava/lang/ClassLoader;", class_signature_ptr) == 0 ){
    //     spdlog::info("entry method {}", method_name);
    //     if (strcmp("loadClass", method_name) == 0) {
    if (strcmp("Lsun/launcher/LauncherHelper;", class_signature_ptr) == 0 ){
        spdlog::info("entry method {}", method_name);
        if (strcmp("checkAndLoadMain", method_name) == 0) {
            jvmti_env->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);

            jclass address_accessor = jni_env->FindClass("logger/AddressAccessor");
            jmethodID address_accessor_init = jni_env->GetStaticMethodID(address_accessor, "init", "()V");
            jni_env->CallStaticVoidMethod(address_accessor, address_accessor_init);

            jclass logger_record = jni_env->FindClass("logger/Record");
            jmethodID logger_record_init = jni_env->GetStaticMethodID(logger_record, "init", "()Z");
            bool r = jni_env->CallStaticBooleanMethod(logger_record, logger_record_init);
            if(r) spdlog::debug("logger/Record init successfully!");

            

            vm_init = true;
            int count;
            jclass* klasses;
            jvmti_env->GetLoadedClasses(&count, &klasses);
            for (int i = 0; i < count; ++i) {
                char* name;
                jvmti_env->GetClassSignature(klasses[i], &name, nullptr);
                if (strncmp("Ljava/lang/invoke", name, strlen("Ljava/lang/invoke")) == 0) {
                    continue;
                }
                //if (strcmp(name, "Ljava/lang/Object;") == 0) {
                    spdlog::debug("before emit retransform class {}", name);
                    jboolean modifiable;
                    jvmti_env->IsModifiableClass(klasses[i], &modifiable);
                    if (!modifiable) {
                        continue;
                    }
                    auto res = jvmti_env->RetransformClasses(1, klasses+i);
                    spdlog::debug("after emit retransform class {} with err code {}", name, res);
                    //assert(res == 0);
                //}
                spdlog::debug("class sig: {}", name);

            }
            jvmti_env->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);

        }

    }
    jvmti_env->Deallocate((unsigned char*)class_signature_ptr);
    jvmti_env->Deallocate((unsigned char*)method_name);
}

// 退出方法时的回调方法
static void JNICALL
MethodExit(jvmtiEnv *jvmti_env,
           JNIEnv *jni_env,
           jthread thread,
           jmethodID method,
           jboolean was_popped_by_exception,
           jvalue return_value)
{
    // 获取执行类
    jclass clazz;
    jvmti_env->GetMethodDeclaringClass(method, &clazz);
    // 获执行类的签名
    char *class_signature_ptr;
    jvmti_env->GetClassSignature(clazz, &class_signature_ptr, NULL);
    //过滤非本工程类信息
    string::size_type idx;
    // 关键字查找，过滤包名，只打印指定包名下的方法信息
    idx = (string(class_signature_ptr)).find(filter_package);
    if (idx != 1)
    {
        return;
    }
    char *method_name_ptr;
    char *method_signaturn_ptr;
    jvmti_env->GetMethodName(method, &method_name_ptr, &method_signaturn_ptr, NULL);
    //fprintf(stdout, "退出方法: %s -> %s\n", method_name_ptr, method_signaturn_ptr);
}

// 退出虚拟机时的回调方法
static void JNICALL
VMDeath(jvmtiEnv *jvmti_env,
        JNIEnv *jni_env)
{
        // pid_t pid = get_tid();
        ASMClient client;
        client.send_header(EXIT);
    //    
    //    //printf("%s %s\n", np, sp) ;
        //fprintf(stdout, "[VM_EXIT] at [proc-%d]\n", pid);

    //fprintf(stdout, "退出虚拟机\n");
}
static void JNICALL
FieldModification(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method,
            jlocation location,
            jclass field_klass,
            jobject object,
            jfieldID field,
            char signature_type,
            jvalue new_value)
{
    //if (!recorder) 
    //{
    //    pid_t tid = get_tid();
    //    recorder = new Recorder((1<<20), tid);
    //}

    ////if (object != NULL)
    //{
    //    char *np,  *sp;
    //    jvmti_env->GetFieldName(field_klass, field, &np, &sp, NULL);
        pid_t pid = get_tid();
    //    
    //    //printf("%s %s\n", np, sp) ;
        //fprintf(stdout, "[MODIFICATION_THREAD] at [proc-%d]\n", pid);
    //    fprintf(stdout, "[MODIFICATION] at [proc-%d] %p: %p\n", pid, object, new_value.l);
    //}
}
// static void JNICALL
// ClassPrepare(jvmtiEnv *jvmti_env,
//             JNIEnv* jni_env,
//             jthread thread,
//             jclass klass)
// {
//     //pid_t pid = get_tid();
//     //printf("[CLASS_PREPARE] at [proc-%d]\n", pid);
//     jint field_count=0;
//     jfieldID* fields;
//     ////fprintf(stdout, "class %p prepared\n", klass);
//     //char* a, *b;
//     //jvmti_env->GetClassSignature(klass,&a,&b);
//     //printf("%s\n", a);
//     //if (strcmp("Lorg/agent/demo/A;", a) == 0 ) 
//     {
//         jvmti_env->GetClassFields(klass, &field_count, &fields);

//         for (int i = 0; i < field_count; ++i)
//         {
//             char* sp;
//             jvmti_env->GetFieldName(klass, fields[i], nullptr, &sp, nullptr);
//             ////fprintf(stdout, "field id: %p\n", fields[i]);
//             //fprintf(stdout, "[FIELD_SIG] %s\n", sp);
//             if (sp[0] == 'L' || sp[0] == '[')
//             {
//                 jvmti_env->SetFieldModificationWatch(klass, fields[i]);
//             }
//             jvmti_env->Deallocate((unsigned char *)sp);
//         }
//         jvmti_env->Deallocate((unsigned char *)fields);
//     }
//     //jvmti_env->Deallocate((unsigned char *)a);
//     //jvmti_env->Deallocate((unsigned char *)b);
// }

static void JNICALL
ClassPrepare(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jclass klass)
{
   // We need to do this to "prime the pump" and get jmethodIDs primed.
  GetJMethodIDs(klass);
}
// AsyncGetCallTrace needs class loading events to be turned on!
static void JNICALL
ClassLoad(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jclass klass)
{
}

static void JNICALL
GarbageCollectionStart(jvmtiEnv*jvmti_env)
{
    set_gc_start(true);
    //printf("[%ld] gc start\n", nanotime());
}

static void JNICALL
GarbageCollectionFinish(jvmtiEnv*jvmti_env)
{
    set_gc_start(false);
    //printf("[%ld] gc finish\n", nanotime());
}

unordered_set<string> transformed_classes;
static void JNICALL
ClassFileLoadHook(jvmtiEnv *jvmti_env,
                  JNIEnv *jni_env,
                  jclass class_being_redefined,
                  jobject loader,
                  const char *name,
                  jobject protection_domain,
                  jint class_data_len,
                  const unsigned char *class_data,
                  jint *new_class_data_len,
                  unsigned char **new_class_data)
{
    spdlog::debug("enter ClassFileLoadHook");
    if (!name || !class_data) {
        return;
    }
    if (!vm_init) {
        return;
    }
    if (transformed_classes.find(name) != transformed_classes.end()) {
        return;
    }
    if ((mode == USEMODE) && candidates.find(name) == candidates.end()) {
        return;
    }
    transformed_classes.insert(name);
    spdlog::debug("enter ClassFileLoadHook for class {}", name);
    if (class_data_len == 0) {
        return;
    }
    if (strlen(name) == 0) {
        return;
    }
    ASMClient client;
    if (mode == BASEMODE) {
        client.send_header(BASE);
    } else if (mode == USEMODE) {
        client.send_header(USE);
    } else if (mode == VALUEMODE) {
        client.send_header(VALUE);
    } else if (mode == CONTAINERMODE) {
        client.send_header(CONTAINER);
    } else {
        spdlog::error("Unknown mode {}", mode);
        throw nullptr;
    }

    client.send_bytecode(name, class_data, class_data_len);
    client.recv_bytecode(new_class_data, new_class_data_len, 
        [=](size_t len, unsigned char** ptr) {jvmti_env->Allocate(len, ptr);});
    spdlog::debug("jvmti loaded class {}", name);
}