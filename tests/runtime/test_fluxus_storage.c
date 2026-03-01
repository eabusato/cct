#include "../../src/runtime/fluxus_runtime.h"

#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void test_fluxus_init_free(void) {
    cct_fluxus_t flux;
    cct_rt_fluxus_init(&flux, sizeof(int));
    if (flux.len != 0 || flux.capacity != 0 || flux.data != NULL) fail("init state invalid");
    cct_rt_fluxus_free(&flux);
}

static void test_fluxus_reserve(void) {
    cct_fluxus_t flux;
    cct_rt_fluxus_init(&flux, sizeof(int));
    cct_rt_fluxus_reserve(&flux, 10);
    if (flux.capacity < 10) fail("reserve did not grow capacity");
    cct_rt_fluxus_free(&flux);
}

static void test_fluxus_push_len_get(void) {
    cct_fluxus_t flux;
    cct_rt_fluxus_init(&flux, sizeof(int));
    int a = 7;
    cct_rt_fluxus_push(&flux, &a);
    if (cct_rt_fluxus_len(&flux) != 1) fail("push did not increment len");
    int *p = (int*)cct_rt_fluxus_get(&flux, 0);
    if (!p || *p != 7) fail("get returned wrong value");
    cct_rt_fluxus_free(&flux);
}

static void test_fluxus_push_many(void) {
    cct_fluxus_t flux;
    cct_rt_fluxus_init(&flux, sizeof(int));
    for (int i = 0; i < 32; i++) {
        cct_rt_fluxus_push(&flux, &i);
    }
    if (cct_rt_fluxus_len(&flux) != 32) fail("len mismatch after many pushes");
    for (int i = 0; i < 32; i++) {
        int *p = (int*)cct_rt_fluxus_get(&flux, (size_t)i);
        if (!p || *p != i) fail("value mismatch after many pushes");
    }
    cct_rt_fluxus_free(&flux);
}

static void test_fluxus_pop(void) {
    cct_fluxus_t flux;
    cct_rt_fluxus_init(&flux, sizeof(int));
    int a = 10;
    int b = 20;
    int out = 0;
    cct_rt_fluxus_push(&flux, &a);
    cct_rt_fluxus_push(&flux, &b);
    cct_rt_fluxus_pop(&flux, &out);
    if (out != 20) fail("first pop mismatch");
    cct_rt_fluxus_pop(&flux, &out);
    if (out != 10) fail("second pop mismatch");
    cct_rt_fluxus_free(&flux);
}

static void test_fluxus_clear(void) {
    cct_fluxus_t flux;
    cct_rt_fluxus_init(&flux, sizeof(int));
    for (int i = 0; i < 8; i++) cct_rt_fluxus_push(&flux, &i);
    cct_rt_fluxus_clear(&flux);
    if (cct_rt_fluxus_len(&flux) != 0) fail("clear did not reset len");
    cct_rt_fluxus_free(&flux);
}

static void test_fluxus_create_destroy(void) {
    cct_fluxus_t *flux = cct_rt_fluxus_create(sizeof(int));
    if (!flux) fail("create returned null");
    if (flux->len != 0 || flux->capacity != 0 || flux->data != NULL) fail("create init state invalid");
    cct_rt_fluxus_destroy(flux);
}

int main(void) {
    test_fluxus_init_free();
    test_fluxus_reserve();
    test_fluxus_push_len_get();
    test_fluxus_push_many();
    test_fluxus_pop();
    test_fluxus_clear();
    test_fluxus_create_destroy();
    printf("ok\n");
    return 0;
}
