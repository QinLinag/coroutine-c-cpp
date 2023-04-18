#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#if __APPLE__ & __MACH__
        #include <sys/ucontext.h>
#else
        #include <ucontext.h>
#endif

#define STACK_SIZE (1024 * 1024)
#define DEFAULT_COROUTINE 16

struct coroutine;

struct schedule {
    char stack[STACK_SIZE];
    ucontext_t main;
    int nco;
    int cap;
    int running;
    struct coroutine** co;
};

struct coroutine {
    coroutine_func func;
    void* ud;
    ucontext_t ctx;
    struct schedule* sch;
    ptrdiff_t cap;  //long int
    ptrdiff_t size;
    int status;
    char* stack;
};

struct coroutine* _co_new(struct schedule* S, 
                        coroutine_func func, void* ud) {
    struct coroutine* co = (struct coroutine*)malloc(sizeof(struct coroutine));
    co->func = func;
    co->ud = ud;
    co->sch = S;
    co->cap = 0;
    co->size = 0;
    co->status  = COROUTINE_READY;
    co->stack = NULL;
}

void _co_delete(struct coroutine* co) {
    if(co) {
        if(co->stack) {
            free(co->stack);
            co->stack = nullptr;
            free(co);
            co = nullptr;
        }
    }
}


struct schedule* coroutine_open(void) {
    struct schedule* S = (struct schedule*)malloc(sizeof(struct schedule));
    S->nco = 0;
    S->cap = DEFAULT_COROUTINE;
    S->running = -1;
    void* temp = malloc(sizeof(struct coroutine) * S->cap);
    void** temp2 = &temp;
    S->co = (struct coroutine**)temp2;
    memset(S->co, 0, sizeof(struct coroutine*) * S->cap);
    return S;
}

void coroutine_close(struct schedule* S) {
    for(int i = 0; i < S->cap; ++i) {
        struct coroutine* co = S->co[i];
        if(co) {
            _co_delete(co);
        }
    }
    free(S->co);
    S->co = nullptr;
    free(S);
    S = nullptr;
}

int coroutine_new(struct schedule* S, 
                        coroutine_func func, void* ud) {
    struct coroutine* co = _co_new(S, func, ud);
    if(S->nco >= S->cap) {
        int id = S->cap;
        void* temp = realloc(S->co, S->cap * 2 * sizeof(struct coroutine));
        void** temp2 = &temp;
        S->co = (struct coroutine**)temp2;
        S->co[S->cap] = co;
        S->cap *= 2;
        ++S->nco;
        return id;  
    } else {
        for(int i = 0; i < S->cap; ++i) {
            int id = (i + S->nco) % S->cap;
            if(S->co[id] == nullptr) {
                S->co[id] = co;
                ++S->nco;
                return id;
            }
        }
    }
    assert(0);
    return -1;
}

void coroutine_resume(struct schedule*, int id);

int coroutine_status(struct schedule*, int id);

int coroutine_running(struct schedule*);
void coroutine_yield(struct schedule*);
