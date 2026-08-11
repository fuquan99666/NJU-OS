#include "common.h"

// PMM : Physical Memory Management

// 我说梦回ics有没有懂的

// 使用互斥锁来避免多线程同时访问内存管理器
// 恰好mpe.c中提供了atomic_xchg函数，可以用来实现自旋锁
static int lock = 0;

static void acquire_lock() {
    while (atomic_xchg(&lock, 1) == 1) {
        // busy wait
    }
}

static void release_lock() {
    atomic_xchg(&lock, 0);
}

// 块元数据头部（存储在每块内存的前面）
typedef struct Block_Header {
    size_t size;              // 整个块的大小（包含头部）
    uint8_t used;             // 1=已分配, 0=空闲, 8-bit 应该是1字节，但是为了对齐size
                              // 填充为8字节
    struct Block_Header *next; // 空闲链表指针（仅在空闲时使用）
    struct Block_Header *prev; // 空闲链表指针（仅在空闲时使用）
} Block_Header;  // 一共32字节，加上尾巴的size, 所以元数据总共40字节

// 从数据指针获取块头部
#define HEADER(ptr) ((Block_Header*)((char*)(ptr) - sizeof(Block_Header)))

// 宏：获取块的尾部标记（用于向前合并）
#define FOOTER(block) ((size_t*)((char*)(block) + (block)->size - sizeof(size_t)))

// 宏：获取下一个块的地址
#define NEXT_BLOCK(block) ((Block_Header*)((char*)(block) + (block)->size))

// 宏：获取上一个块的地址（通过尾部标记）
#define PREV_BLOCK(block) ((Block_Header*)((char*)(block) - *(size_t*)((char*)(block) - sizeof(size_t))))


typedef struct List {
    struct Block_Header *head;
    int count; // the number of free memory blocks, 也会对齐到8字节
} List;

// 全局变量
static List *free_list = NULL;     // 空闲链表
int heap_allocated = 0; // 已分配的堆大小（仅初始化时使用）

// 辅助函数：设置尾部标记
static inline void set_footer(Block_Header *block) {
    *FOOTER(block) = block->size;
}

// 辅助函数：检查块是否空闲
static inline int is_free(Block_Header *block) {
    return block != NULL && !block->used;
}

// for s , we need to align it to 2^i, which is the smallest power of 2 that is greater than s
static inline size_t align_size(size_t size) {
    int i = 0;
    while ((1U << i) < size) {
        i++;
    }
    return (1U << i);
}




// 从空闲链表中删除节点
static void remove_from_free_list(Block_Header *block) {
    if (!block) return;
    
    if (block->prev) {
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    } else {
        free_list->head = block->next;
        if (block->next) {
            block->next->prev = NULL;
        }
    }
    free_list->count--;
}

// 插入到空闲链表（按地址排序）
static void insert_to_free_list(Block_Header *block) {
    if (!block || !free_list) return;
    
    Block_Header *curr = free_list->head;
    Block_Header *prev = NULL;
    
    // 按地址从小到大插入
    while (curr && (uintptr_t)curr < (uintptr_t)block) {
        prev = curr;
        curr = curr->next;
    }
    
    block->next = curr;
    block->prev = prev;
    
    if (prev != NULL) {
        prev->next = block;
    } else {
        free_list->head = block;
    }
    if (curr) {
        curr->prev = block;
    }
    free_list->count++;
}

// 合并相邻空闲块
static void merge_blocks(Block_Header *block) {
    // 1. 向前合并（检查前一个块）

    Block_Header *prev_block = PREV_BLOCK(block);

    if (prev_block && (char*)block != (char*)free_list->head && !prev_block->used) {
        // 前一个块空闲，合并
        prev_block->size += block->size;
        set_footer(prev_block);
        
        remove_from_free_list(block);
        block = prev_block; // 更新当前块为合并后的块
    }
    
    // 2. 向后合并（检查后一个块）
    Block_Header *next_block = NEXT_BLOCK(block);
    if ((char*)next_block < (char*)heap.end && !next_block->used) {
        // 后一个块空闲，合并
        block->size += next_block->size;
        set_footer(block);
        
        // 删除后一个块
        remove_from_free_list(next_block);
    }
}


static void  __attribute__((unused)) print_free_list() {
    printf("------- Free list dump -------\n");
    Block_Header *curr = free_list->head;
    while (curr != NULL) {
        printf("Free block at %p, size = %zu bytes\n", curr, curr->size);
        curr = curr->next;
    }
    printf("------- End of dump -------\n");
}


static void *kalloc(size_t size) {
    // TODO
    // You can add more .c files to the repo.

    
    // we can reject the request if the size is larger than 16MiB
    if (size == 0 || size > 16 * 1024 * 1024) {
        return NULL;
    }

    acquire_lock();
    
    // 从空闲链表中查找合适的块（首次适应）
    Block_Header *curr = (Block_Header*)free_list->head;

    int enough_size = 0;

    // 计算对齐后的大小（包含头部和尾部元数据）
    size_t total_size = size + sizeof(Block_Header) + sizeof(size_t); 

    // Free list head is not available ...
    if (curr == NULL) {
        printf("Free list is empty !!!\n");
        release_lock();
        return NULL;
    }

    // print_free_list();

    while (curr != NULL) {

        // 判断是否有足够的空间，以及对齐问题（考虑头尾元数据）
        // 为了简化，先不考虑对齐问题
        //uintptr_t aligned_start = align_size((uintptr_t)(curr + sizeof(Block_Header))); 
        //enough_size = (aligned_start + size <= (uintptr_t)FOOTER(curr));
        enough_size = (curr->size >= total_size);
        
        if (enough_size) {
            // 找到了合适的块
            size_t remain = curr->size - total_size;
            
            // 如果剩余空间足够大，分裂
            if (remain >= sizeof(Block_Header) + sizeof(size_t) + 8) {
                // 创建新空闲块
                Block_Header *new_block = (Block_Header*)((char*)curr + total_size);
                new_block->size = remain;
                new_block->used = 0;
                
                // 顺手扔进空闲链表，就不多调用一次insert了
                new_block->next = curr->next;
                new_block->prev = curr;
                set_footer(new_block);
                
                // 更新当前块
                curr->size = total_size;
                curr->next = new_block;
            } else {
                // 剩余空间太小，一起分配出去
            }
            
            remove_from_free_list(curr);
            
            // 标记为已分配
            curr->used = 1;
            curr->next = NULL;
            curr->prev = NULL;
            set_footer(curr);

            printf("Allocated %zu bytes at %p\n", size, curr + 1);
            
            // 返回数据区（跳过头部）
            release_lock();

            return (void*)(curr + 1);
        }
        curr = curr->next;
    }

    // 没有找到合适的块
    printf("Loop through free list but can't find any enough block !\n");

    
    release_lock();
    return NULL;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
    if (!ptr) {
        printf("kfree: NULL pointer!\n");
        return;
    }

    acquire_lock();

    // print_free_list();
    
    // 获取块头部（数据区前面的元数据）
    Block_Header *block = (Block_Header*)ptr - 1;
    
    // 安全检查：防止 double-free
    if (!block->used) {
        // 实验要求：调用者保证合法，这里可以 panic
        printf("kfree: double free or invalid pointer!\n");
        release_lock();
        return;
    }

    
    // 标记为空闲
    block->used = 0;
 
    // 插入空闲链表
    insert_to_free_list(block);   

    // 合并相邻空闲块
    merge_blocks(block);

    printf("Freed memory at %p\n", ptr);

    release_lock();
}

#ifndef TEST
static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    );

    // 文档中要求我们管理堆的数据结构要在堆中分配，不能占用静态内存 。。。
    // 所以我们在pmm初始化时，先在堆中分配一块区域来存储我们管理堆的链表数据结构 。。。
    
    // 1. 初始化 free_list 结构（在堆区开头）, 直接分配
    free_list = heap.start;

    free_list->head = NULL;
    free_list->count = 0;
    
    // 2. 计算剩余堆空间
    uintptr_t remaining_start = (uintptr_t)heap.start + sizeof(List);
    size_t remaining_size = (uintptr_t)heap.end - remaining_start;
    
    // 3. 将剩余空间初始化为一个大的空闲块
    Block_Header *first_block = (Block_Header*)remaining_start;
    first_block->size = remaining_size;
    first_block->used = 0;  // 空闲
    first_block->next = NULL;
    first_block->prev = NULL;
    set_footer(first_block);
    
    // 4. 加入空闲链表
    free_list->head = first_block;
    free_list->count = 1;
    
    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );
    // for size , its type is size_t, which is unsigned long in 64-bit system
    // but our heap is up to 4GiB, which only needs 32-bit unsigned int to represent.
    // here we just use int for simplicity, maybe exists bugs. 
    printf("Free list at %p, first block at %p, size = %d KB\n",
           free_list, first_block, first_block->size / 1024);
}
#else
static void pmm_init() {
    int HEAP_SIZE = 128 * 1024 * 1024; // 128 MiB
    char *ptr  = malloc(HEAP_SIZE);
    heap.start = ptr;
    heap.end   = ptr + HEAP_SIZE;
    printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);

    // 1. 初始化 free_list 结构（在堆区开头）, 直接分配
    free_list = heap.start;

    free_list->head = NULL;
    free_list->count = 0;
    
    // 2. 计算剩余堆空间
    uintptr_t remaining_start = (uintptr_t)heap.start + sizeof(List);
    size_t remaining_size = (uintptr_t)heap.end - remaining_start;
    
    // 3. 将剩余空间初始化为一个大的空闲块
    Block_Header *first_block = (Block_Header*)remaining_start;
    first_block->size = remaining_size;
    first_block->used = 0;  // 空闲
    first_block->next = NULL;
    first_block->prev = NULL;
    set_footer(first_block);
    
    // 4. 加入空闲链表
    free_list->head = first_block;
    free_list->count = 1;
    
    // for size , its type is size_t, which is unsigned long in 64-bit system
    // but our heap is up to 4GiB, which only needs 32-bit unsigned int to represent.
    // here we just use int for simplicity, maybe exists bugs. 
    printf("Free list at %p, first block at %p, size = %d KB\n",
           free_list, first_block, first_block->size / 1024);
}
#endif

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};