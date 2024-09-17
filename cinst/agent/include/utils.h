#pragma once
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
inline pid_t get_tid() 
{
    return syscall(SYS_gettid);
}

inline uint64_t nanotime()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec*((uint64_t)1e9)+ts.tv_nsec;
}
enum MODE {
    INVALID = -1,
    BASEMODE = 0,
    USEMODE = 1,
    VALUEMODE = 2,
    CONTAINERMODE = 3
};
enum COMMAND {
    HELLO = 0,
    BASE = 1,
    USE = 2,
    VALUE = 3,
    CONTAINER = 4,
    EXIT = 5
};