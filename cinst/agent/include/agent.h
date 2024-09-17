#ifndef CUSTOMER_AGENT_H
#define CUSTOMER_AGENT_H
#include "jvmti.h"
#include <dlfcn.h>
#include <execinfo.h>

// A copy of the ASGCT data structures.
typedef struct {
    jint lineno;                      // line number in the source file
    jmethodID method_id;              // method executed in this frame
} ASGCT_CallFrame;

typedef struct {
    JNIEnv *env_id;                   // Env where trace was recorded
    jint num_frames;                  // number of frames in this trace
    ASGCT_CallFrame *frames;          // frames
} ASGCT_CallTrace;

typedef void (*ASGCTType)(ASGCT_CallTrace *, jint, void *);

// JVM 通过回调该方法启动 Agent
JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options, void *reserved);

// 进入方法是时的回调方法
static void JNICALL
MethodEntry(jvmtiEnv *jvmti_env,
            JNIEnv *jni_env,
            jthread thread,
            jmethodID method);

// 退出方法时的回调方法
static void JNICALL
MethodExit(jvmtiEnv *jvmti_env,
           JNIEnv *jni_env,
           jthread thread,
           jmethodID method,
           jboolean was_popped_by_exception,
           jvalue return_value);

// 退出虚拟机时的回调方法
static void JNICALL
VMDeath(jvmtiEnv *jvmti_env,
        JNIEnv *jni_env);
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
            jvalue new_value);
static void JNICALL
ClassPrepare(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jclass klass);
static void JNICALL
ClassLoad(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jclass klass);
static void JNICALL
GarbageCollectionStart(jvmtiEnv*jvmti_env);

static void JNICALL
GarbageCollectionFinish(jvmtiEnv*jvmti_env);
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
                  unsigned char **new_class_data);
#endif