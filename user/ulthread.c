/* CSE 536: User-Level Threading Library */
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "user/ulthread.h"

/* Standard definitions */
#include <stdbool.h>
#include <stddef.h> 

void ulthread_schedule(void);

struct ulthread threads[MAXULTHREADS];
enum ulthread_scheduling_algorithm sch_algo;
int cur_tid;  
int prev_tid;
int thread_count =0;

/* Get thread ID */
int get_current_tid(void) {
    return cur_tid;
}

uint64 get_start_time(void) {
    uint64 t = ctime();
    return t;
}

/* Thread initialization */
void ulthread_init(int schedalgo) {
    sch_algo  = schedalgo;
    struct ulthread *schd_thread = &threads[0];
    schd_thread->threadId = 0;
    memset(&schd_thread->context, 0, sizeof(schd_thread->context));
    // schd_thread->start_add = (uint64) ulthread_schedule;
    // schd_thread->context.ra = (uint64) ulthread_schedule;
    thread_count++;
    cur_tid = 0;
    prev_tid = 0;
}

/* Thread creation */
bool ulthread_create(uint64 start, uint64 stack, uint64 args[], int priority) {
    struct ulthread *new_thread = &threads[thread_count];
    new_thread->threadId = thread_count;
    thread_count++;
    new_thread->start_add = start;
    new_thread->stack = stack;
    new_thread->priority = priority;
    new_thread->create_time = get_start_time();
    new_thread->state = RUNNABLE;
    memset(&new_thread->context, 0, sizeof(new_thread->context));
    new_thread->context.ra = start;
    new_thread->context.sp = stack;
    new_thread->context.a0 = args[0];
    new_thread->context.a1 = args[1];
    new_thread->context.a2 = args[2];
    new_thread->context.a3 = args[3];
    new_thread->context.a4 = args[4];
    new_thread->context.a5 = args[5];

    /* Please add thread-id instead of '0' here. */
    printf("[*] ultcreate(tid: %d, ra: %p, sp: %p)\n", new_thread->threadId, start, stack);
    return false;
}

int _round_robin() {
    int nxt = -1; 
    int itr = (prev_tid + 1)%thread_count;
    int checked = 0;
    while(checked < thread_count) {
        if(itr == 0) {
            itr = (itr +1)%thread_count;
            checked++;
            continue;
        }
        if(threads[itr].state == RUNNABLE && (nxt == -1  || threads[nxt].state == YIELD)) {
            checked++;
            nxt = itr;
        }
        if(threads[itr].state == YIELD)  {
            if(nxt == -1 || (threads[nxt].state == YIELD))
                nxt = itr;
        }
        itr = (itr +1)%thread_count;
        checked++;
    }
    for(int i=1; i<thread_count; i++) {
        if(threads[i].state == YIELD) {
            threads[i].state = RUNNABLE;
        }
    }
    return nxt;
}

int _priority_sch() {
    int nxt = -1;
    for(int i=1; i< thread_count; i++) {
        switch (threads[i].state)
        {
        case RUNNABLE:
            if(nxt == -1 || threads[nxt].priority < threads[i].priority || threads[nxt].state == YIELD)
                nxt = i;
            break;
        case YIELD:
            if(nxt == -1 || (threads[nxt].state == YIELD && threads[nxt].priority < threads[i].priority))
                nxt = i;
            break;
        case FREE:
        default:
            break;
        }
    }
    for(int i=1; i<thread_count; i++) {
        if(threads[i].state == YIELD) {
            threads[i].state = RUNNABLE;
        }
    }
    return nxt;
}

int _fsfc() {
    int nxt = -1;
    for(int i=1; i< thread_count; i++) {
        switch (threads[i].state)
        {
        case RUNNABLE:
            if(nxt == -1 || threads[nxt].create_time > threads[i].create_time || threads[nxt].state == YIELD)
                nxt = i;
            break;
        case YIELD:
            if(nxt == -1 || (threads[nxt].state == YIELD && threads[nxt].create_time > threads[i].create_time))
                nxt = i;
            break;
        case FREE:
        default:
            break;
        }
    }
    for(int i=1; i<thread_count; i++) {
        if(threads[i].state == YIELD) {
            threads[i].state = RUNNABLE;
        }
    }
    return nxt;
}

int findNextThread() {
    switch (sch_algo)
    {
    case ROUNDROBIN:
        return _round_robin();
    case PRIORITY:
        return _priority_sch();
    case FCFS:
        return _fsfc();
    default:
        return -1;
    }
}

/* Thread scheduler */
void ulthread_schedule(void) {
    for(;;)  {
        int nextThread = findNextThread();
        if(nextThread <= 0) break; 
        /* Add this statement to denote which thread-id is being scheduled next */
        printf("[*] ultschedule (next tid: %d)\n", nextThread);
        cur_tid = nextThread;
        // Switch between thread contexts
        ulthread_context_switch(&threads[0].context, &threads[nextThread].context);
        // ulthread_schedule();
    }
}

/* Yield CPU time to some other thread. */
void ulthread_yield(void) {
    threads[cur_tid].state = YIELD;
    /* Please add thread-id instead of '0' here. */
    printf("[*] ultyield(tid: %d)\n", cur_tid);
    prev_tid = cur_tid;
    cur_tid = 0;
    ulthread_context_switch(&threads[prev_tid].context, &threads[0].context);
}

/* Destroy thread */
void ulthread_destroy(void) {
    threads[cur_tid].state = FREE;
    printf("[*] ultdestroy(tid: %d)\n", cur_tid);
    prev_tid = cur_tid;
    cur_tid = 0;
    ulthread_context_switch(&threads[prev_tid].context, &threads[0].context);
}
