#include "kernel.h"
#include "thread.h"

// 为了在本地方便测试，我们新建了一个test文件夹
// am.h 中定义了 一些将要用到的函数和变量
// 我们将利用之前课程中用过的thread.h来创建多线程，而不是使用mpe.c的那一套逻辑
// 这样我们就摆脱了abstract-machine那一堆玩意，怎么简单怎么来，重点就是测试pmm.c的功能

Area heap = {};

static void* alloc_test(size_t size) {
    void *ptr = pmm->alloc(size);
    if (ptr == NULL) {
        printf("Allocation failed for size %zu bytes!\n", size);
        return NULL;
    } else {
        return ptr;
    }
}

static void free_test(void *ptr) {
    if (ptr == NULL) {
        printf("Free failed: NULL pointer!\n");
        return;
    }
    pmm->free(ptr);
}

static void entry(int tid) {

    void* ptr = alloc_test(128);

    sleep(0.5);

    free_test(ptr);

    sleep(0.5);

    ptr = alloc_test(256);

    sleep(0.5);

    free_test(ptr);    

    sleep(0.5);

    ptr = alloc_test(512);

    sleep(0.5);

    free_test(ptr);

    sleep(0.5);

    ptr = alloc_test(1024);

    sleep(0.5);

    free_test(ptr);
}


// extract all printf's output to a file, 
// then try to analyze the output to see if there are one byte allocated to many times in one time .

static void judge() {
    FILE *file = fopen("output.txt", "r");
    if (!file) {
        fprintf(stderr, "Failed to open output.txt\n");
        return;
    }

    typedef struct {
        void *ptr;
        int alive;
    } record_t;

    record_t *records = NULL;
    size_t count = 0, cap = 0;
    char line[256];
    int ok = 1;

    while (fgets(line, sizeof(line), file)) {
        void *ptr = NULL;
        size_t size = 0;

        // 只解析 Allocated 和 Freed memory 行
        if (sscanf(line, "Allocated %zu bytes at %p", &size, &ptr) == 2) {
            // 检查是否重复分配（同一指针还没释放又分配）
            for (size_t i = 0; i < count; i++) {
                if (records[i].alive && records[i].ptr == ptr) {
                    fprintf(stderr, "ERROR: duplicate alloc %p without free\n", ptr);
                    ok = 0;
                }
            }

            if (count >= cap) {
                cap = cap ? cap * 2 : 64;
                records = realloc(records, cap * sizeof(record_t));
            }
            records[count++] = (record_t){.ptr = ptr, .alive = 1};
        }

        if (sscanf(line, "Freed memory at %p", &ptr) == 1) {
            // 查找匹配的分配记录
            int found = 0;
            for (size_t i = 0; i < count; i++) {
                if (records[i].alive && records[i].ptr == ptr) {
                    records[i].alive = 0;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "ERROR: free unknown pointer %p\n", ptr);
                ok = 0;
            }
        }
    }

    fclose(file);

    // 检查泄漏
    int leaked = 0;
    for (size_t i = 0; i < count; i++) {
        if (records[i].alive) {
            if (!leaked) fprintf(stderr, "Leaked pointers:\n");
            fprintf(stderr, "  %p\n", records[i].ptr);
            leaked++;
        }
    }

    if (ok && !leaked) {
        printf("judge: OK, %zu allocations, all freed\n", count);
    } else {
        if (leaked) fprintf(stderr, "%zu leak(s) detected\n", leaked);
        fprintf(stderr, "judge: FAILED\n");
    }

    free(records);
}

static void goodbye()      { printf("End.\n"); }

int main() {
    // 将 printf 的输出重定向到一个文件中
    char *output_file = "output.txt";
    freopen(output_file, "w", stdout);
    pmm->init();
    for (int i = 0; i < 4; i++) {
        // printf("Starting thread %d\n", i);
        create(entry);
    }
    join();
    goodbye();
    judge();
    return 0;
}

