#include "coroutine.h"


#if __APPLE__ & __MACH__
        #include <sys/ucontext.h>
#else
        #include <ucontext.h>
#endif



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
    co->stack = nullptr;
    return co;
}

static void _co_delete(struct coroutine* co) {
    if(co) {
        if(co->stack) {
            free(co->stack);
            co->stack = nullptr;
            free(co);
            co = nullptr;
        }
    }
}


struct schedule* coroutine_open() {
    struct schedule* S = (struct schedule*)malloc(sizeof(struct schedule));
    S->nco = 0;
    S->cap = DEFAULT_COROUTINE;
    S->running = -1;
    S->co = (struct coroutine**)malloc(sizeof(struct coroutine) * S->cap);
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
        S->co = (struct coroutine**)realloc(S->co, S->cap * 2 * sizeof(struct coroutine));
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

static void mainfunc(uint32_t low32, uint32_t hi32) {
    uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
    struct schedule* S = (struct schedule*)ptr;
    int id = S->running;
    struct coroutine* C = S->co[id];
    C->func(S, C->ud);
    _co_delete(C);
    S->co[id] = nullptr;
    --S->nco;
    S->running = -1;
}



void coroutine_resume(struct schedule* S, int id) {
    std::cout << "coroutine_resume 1" << std::endl;
    assert(S->running == -1);
    assert(id >= 0 && id < S->cap);
    struct coroutine* C = S->co[id];
    if(C == nullptr) {
        return;
    }
    int status = C->status;
    switch(status) {
        case COROUTINE_READY:
            {
                getcontext(&C->ctx);
                C->ctx.uc_stack.ss_sp = S->stack;
                C->ctx.uc_stack.ss_size = STACK_SIZE;
                C->ctx.uc_link = &S->main;
                S->running = id;
                C->status = COROUTINE_RUNNING;
                uintptr_t ptr = (uintptr_t)S;
                makecontext(&C->ctx, (void(*)(void))mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
                swapcontext(&S->main, &C->ctx); //将当前上下文信息保存到S->main中， 然后恢复C->ctx上下文信息，
                break;
            }
        case COROUTINE_SUSPEND:
            {
                memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);
                S->running = id;
                C->status = COROUTINE_RUNNING;
                swapcontext(&S->main, &C->ctx);
                break;
            }
        default:
            assert(0);
    }

}

static void _save_stack(struct coroutine* C, char* top) {
    char dummy = 0;
    assert(top - &dummy <= STACK_SIZE);
    if(C->cap < top - &dummy) {
        free(C->stack);
        C->cap = top - &dummy;
        C->stack = (char*)malloc(C->cap);
    }
    C->size = top - &dummy;
    memcpy(C->stack, &dummy, C->size);
}

int coroutine_status(struct schedule* S, int id) {
    assert(id >= 0 && id < S->cap);
    if(S->co[id] == nullptr) {
        return COROUTINE_DEAD;
    }
    return S->co[id]->status;
}

int coroutine_running(struct schedule* S) {
    return S->running;
}

void coroutine_yield(struct schedule* S) {
    int id = S->running;
    assert(id >= 0);
    struct coroutine* C = S->co[id];
    assert((char*)&C > S->stack);
    _save_stack(C, S->stack + STACK_SIZE);
    C->status = COROUTINE_SUSPEND;
    S->running = -1;
    swapcontext(&C->ctx, &S->main);  //将当前上下文信息保存到C->ctx ，然后恢复S->main上下文信息，
}
