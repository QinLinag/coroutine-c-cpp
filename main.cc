#include "coroutine.h"
#include <iostream>
#include <stdio.h>


struct args{
    int n;
};

static void foo(struct schedule* S, void* ud) {
    struct args* arg = (struct args*)ud;
    int start = arg->n;
    for(int i =0; i < 5; ++i) {
        printf("coroutine %d : %d\n", coroutine_running(S), start + i);
        coroutine_yield(S);
    }
}

static void test(struct schedule* S) {
    struct args arg1 = { 0 };
    struct args arg2 = { 100 };

    int co1 = coroutine_new(S, foo, &arg1);
    int co2 = coroutine_new(S, foo, &arg2);
    std::cout << "main start" << std::endl;
    while(coroutine_status(S, co1) 
                    && coroutine_status(S,co2)) {
        std::cout << "main during" << std::endl;
        coroutine_resume(S, co1);
        coroutine_resume(S, co2);
    }
    std::cout << "main end" << std::endl;
}

int main() {
    struct schedule* S = coroutine_open();
    test(S);
    coroutine_close(S);
    return 0;
}