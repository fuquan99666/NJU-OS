#include <kernel.h>
#include "thread.h"

// 为了在本地方便测试，我们新建了一个test文件夹
// am.h 中定义了 一些将要用到的函数和变量
// 我们将利用之前课程中用过的thread.h来创建多线程，而不是使用mpe.c的那一套逻辑
// 这样我们就摆脱了abstract-machine那一堆玩意，怎么简单怎么来，重点就是测试pmm.c的功能


// 有待改进的点：
// 1. 目前没有对malloc的内存分配进行对齐处理
// 2. 目前的测试函数不够严格，期望后面可以修改成ptr + size 的区间测试
// 3. 当前多处理器正确性应该是没问题的，直接上了一把大锁，后期期望通过一些方法优化性能
// 4. 当前测试不够hard, 后期希望添加一些更复杂的测试，比如while循环中不断分配和释放，以期找到隐藏bug
// 5. 具体性能提升时期望使用profiler来分析，看看是否有必要优化

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

// extract all printf's output to a file, 
// then try to analyze the output to see if there are one byte allocated to many times in one time .

static void judge() {
    FILE *file = fopen("output.txt", "r");
    if (!file) {
        fprintf(stderr, "judge: cannot open output.txt\n");
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

        // 匹配分配
        if (sscanf(line, "Allocated %zu bytes at %p", &size, &ptr) == 2) {
            // 检查重复分配（同一指针还活着）
            for (size_t i = 0; i < count; i++) {
                if (records[i].alive && records[i].ptr == ptr) {
                    fprintf(stderr, "judge: duplicate alloc %p\n", ptr);
                    ok = 0;
                    break;
                }
            }

            // 扩容
            if (count >= cap) {
                cap = cap ? cap * 2 : 64;
                record_t *new_rec = realloc(records, cap * sizeof(record_t));
                if (!new_rec) {
                    fprintf(stderr, "judge: out of memory\n");
                    fclose(file);
                    free(records);
                    return;
                }
                records = new_rec;
            }

            records[count].ptr = ptr;
            records[count].alive = 1;
            count++;
            continue;
        }

        // 匹配释放
        if (sscanf(line, "Freed memory at %p", &ptr) == 1) {
            int found = 0;
            for (size_t i = 0; i < count; i++) {
                if (records[i].alive && records[i].ptr == ptr) {
                    records[i].alive = 0;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "judge: double free or unknown pointer %p\n", ptr);
                ok = 0;
            }
            continue;
        }
    }

    fclose(file);

    // 统计泄漏
    size_t leaked = 0;
    for (size_t i = 0; i < count; i++) {
        if (records[i].alive) {
            if (leaked == 0) fprintf(stderr, "Leaked pointers:\n");
            fprintf(stderr, "  %p\n", records[i].ptr);
            leaked++;
        }
    }

    if (ok && leaked == 0) {
        // 由于我们将stdout 重定向到output.txt，所以这里用stderr输出judge结果
        fprintf(stderr, "judge: OK, %zu allocations, all freed\n", count);
    } else {
        if (leaked) fprintf(stderr, "%zu leak(s) detected\n", leaked);
        fprintf(stderr, "judge: FAILED\n");
    }

    free(records);
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

static void entry_1(int tid) {
    
    void* ptr = alloc_test(77);

    sleep(0.5);

    free_test(ptr);

    sleep(0.5);

    ptr = alloc_test(298);

    sleep(0.5);

    free_test(ptr);    

    sleep(0.5);

    ptr = alloc_test(888);

    sleep(0.5);

    free_test(ptr);

    sleep(0.5);

    ptr = alloc_test(1923);

    sleep(0.5);

    free_test(ptr);

    ptr = alloc_test(382);

    free_test(ptr);
}

static void entry_2(int tid) {
    // 多次循环
    for (int i = 0; i < 10000; i++) {
        void* ptr = alloc_test(64 + (rand() % 128)); // 随机分配 64~191 字节
        sleep(0.01);
        free_test(ptr);
    }
}

static void goodbye()      { printf("End.\n"); }

static void do_test_0() { freopen("output.txt", "w", stdout); pmm->init(); entry(0); fflush(stdout); judge();}
static void do_test_1() { freopen("output.txt", "w", stdout); pmm->init(); entry_1(0); fflush(stdout); judge();}
static void do_test_2() { freopen("output.txt", "w", stdout); pmm->init(); entry_2(0); fflush(stdout); judge();}
static void do_test_3() { freopen("output.txt", "w", stdout); pmm->init(); entry(0); fflush(stdout); judge();}
static void do_test_4() { freopen("output.txt", "w", stdout); pmm->init(); for (int i = 0; i < 4; i++) { create(entry); } join(); fflush(stdout); judge(); }
int main(int argc, char *argv[]) {
  if (argc < 2) exit(1);
  switch(atoi(argv[1])) {
    case 0: do_test_0(); break;
    case 1: do_test_1(); break;
    case 2: do_test_2(); break;
    case 4: do_test_4(); break;
    default: break;
  }
}

//int main() {
    //// 将 printf 的输出重定向到一个文件中
    //char *output_file = "output.txt";
    //freopen(output_file, "w", stdout);
    //pmm->init();
    //for (int i = 0; i < 4; i++) {
        //// printf("Starting thread %d\n", i);
        //create(entry);
    //}
    //join();
    //goodbye();

    //// this is neccessary to ensure all output is written to the file 
    //// 不然就会出现 fopen 成功 ，但是文件中没有内容的情况
    //fflush(stdout); 

    //judge();

    //return 0;
//}

