#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <unordered_map>
#include <time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <memory>
#include "recorder.hpp"
volatile bool _gc_start = false;
thread_local std::unique_ptr<Recorder<EventRecord<0>>> event_recorder = nullptr;
void set_gc_start(bool v) {
    // if (v)
    // fprintf(stderr, "[GC] start\n");
    // else
    // fprintf(stderr, "[GC] end\n");
    if (!event_recorder) {
        event_recorder = std::make_unique<Recorder<EventRecord<0>>>(1, syscall(SYS_gettid), getpid());
    }

    if (v) {
        EventRecord<0> r;
        r.event_id = EventRecord<0>::GC_START_EVENT;
        r.time = nanotime();
        event_recorder->add(r);
    } else {
        EventRecord<0> r;
        r.event_id = EventRecord<0>::GC_END_EVENT;
        r.time = nanotime();
        event_recorder->add(r);

    }
    _gc_start = v;
}

constexpr int GC_BUFFER_SIZE = (1<<8);


//thread_local AddrChangeRecorder recorder(syscall(SYS_gettid));
thread_local std::unique_ptr<Recorder<GCRecord<0>>> recorder = nullptr;

typedef void *(*memmove_t)(void *, const void *, size_t);
memmove_t real_memmove = NULL;


//SpinLock relocation_map_lock;
//std::unordered_map<void *, relocation_info_t> relocation_map = {};

static void setup_real_functions()
{
    real_memmove = (memmove_t)dlsym(RTLD_NEXT, "memmove");
    if (NULL == real_memmove)
    {
        fprintf(stderr, "Error in dlsym(): %s\n", dlerror());
    }
}

static inline void *call_real_memmove(void *dest, const void *src, size_t n)
{
    if (real_memmove == NULL)
    {
        setup_real_functions();
    }
    return real_memmove(dest, src, n);
}

void *memmove(void *dest, const void *src, size_t n)
{
    void *ptr = call_real_memmove(dest, src, n);
    if (_gc_start) {
    //fprintf(stderr, "%ld->%ld\n",(uint64_t)src, (uint64_t)dest);
    //if (true) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t timestamp = ts.tv_sec*((uint64_t)1e9)+ts.tv_nsec;
        //m.lock();
        if (!recorder) {
            recorder = std::make_unique<Recorder<GCRecord<0>>>(GC_BUFFER_SIZE, syscall(SYS_gettid), getpid());
        }
        if (!((uint64_t)src & 7)) {
            GCRecord<0> r;
            r.new_addr = (uint32_t)((uint64_t)dest>>3);
            r.org_addr = (uint32_t)((uint64_t)src>>3);
            r.time = timestamp;
            recorder->add(r);    

        }

    } 
    //recorder.add((uint64_t)src, (uint64_t)dest, timestamp);
    //m.unlock();

    return ptr;
}
