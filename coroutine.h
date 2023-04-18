#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include <ucontext.h> 
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <iostream>

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3



#define STACK_SIZE (1024 * 1024)
#define DEFAULT_COROUTINE 16

typedef void (*coroutine_func)(struct schedule*, void* ud);




class Schedule;
class Coroutine {
public:
    void co_delete();
private:
    coroutine_func func;
    void* ud;
    ucontext_t ctx;
    Schedule* sch;
    ptrdiff_t cap;  //long int
    ptrdiff_t size;
    int status;
    char* stack;
};

class Schedule{
public:
    Schedule();
    void coroutine_close();
    int coroutine_new(coroutine_func func, void* ud);
    void coroutine_resume(int id);
    int coroutine_status(int id);
    int coroutine_running();
    void coroutine_yield();

private:
    char stack[STACK_SIZE];
    ucontext_t main;
    int nco;
    int cap;
    int running;
    struct coroutine** co;
};


struct schedule;

struct schedule* coroutine_open(void);
void coroutine_close(struct schedule*);

int coroutine_new(struct schedule*, coroutine_func, void* ud);
void coroutine_resume(struct schedule*, int id);
int coroutine_status(struct schedule*, int id);
int coroutine_running(struct schedule*);
void coroutine_yield(struct schedule*);

#endif