#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "debug.h"
#include "print.h"
#include "interrupt.h"
#include "sync.h"
#include "process.h"
#include "stdio.h"
#include "fs.h"
#include "file.h"

/* pid的位图, 最大支持1024个pid */
uint8_t pid_bitmap_bits[128] = {0};

/* pid池 */
struct pid_pool
{
    struct bitmap pid_bitmap; // pid位图
    uint32_t pid_start;       // 起始pid
    struct lock pid_lock;     // 分配pid锁
} pid_pool;

extern void init(void);
extern void switch_to(struct task_struct *cur, struct task_struct *next);

/*分配pid*/
static pid_t allocate_pid(void)
{
    lock_acquire(&pid_pool.pid_lock);
    int32_t bit_idx = bitmap_scan(&pid_pool.pid_bitmap, 1);
    bitmap_set(&pid_pool.pid_bitmap, bit_idx, 1);
    lock_release(&pid_pool.pid_lock);
    return (bit_idx + pid_pool.pid_start);
}

struct task_struct *idle_thread;     // idle线程, 在系统中没有其他任务需要运行时占用CPU
struct lock pid_lock;                // 分配pid锁
struct task_struct *main_thread;     // 主线程pcb
struct list thread_ready_list;       // 就绪队列
struct list thread_all_list;         // 所有任务队列
static struct list_elem *thread_tag; // 用于保存队列中的线程结点,
                                     //   队列中的结点不是pcb，需要将tag转换为pcb，所以记录tag用
extern void switch_to(struct task_struct *cur, struct task_struct *next);

/* 系统空闲时运行的线程 */
static void idle(void *arg UNUSED)
{
    while (1)
    {
        thread_block(TASK_BLOCKED);
        // 执行hlt时必须要保证目前处在开中断的情况下
        asm volatile("sti\n hlt" : : : "memory"); // hlt在系统中没有其他任务需要运行时占用CPU
    }
}

/*获取当前pcb指针(虚拟地址)*/
struct task_struct *running_thread()
{
    uint32_t esp;
    asm("mov %%esp, %0" : "=g"(esp));
    /*取esp整数部分, 即pcb起始地址*/
    return (struct task_struct *)(esp & 0xfffff000);
}

/* 由kernel_thread去执行functionion(funct_arg)*/
static void kernel_thread(thread_func *function, void *func_arg)
{
    intr_enable(); // 避免时钟中断被屏蔽
    function(func_arg);
}

/*初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack相应的位置*/
void thread_create(struct task_struct *pthread, thread_func function, void *func_arg)
{
    /*先预留中断使用栈的空间，可见thread.h定义的结构*/
    pthread->self_kstack -= sizeof(struct intr_stack);

    /*再留出线程栈空间, 可见thread.h定义*/
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0; // 这四个寄存器执行过程才会有值
}

/* 初始化pid池 */
static void pid_pool_init(void)
{
    pid_pool.pid_start = 1;     // 最小的pid设置为1
    pid_pool.pid_bitmap.bits = pid_bitmap_bits;
    pid_pool.pid_bitmap.btmp_bytes_len = 128;
    bitmap_init(&pid_pool.pid_bitmap);
    lock_init(&pid_pool.pid_lock);
}

/* 释放pid */
void release_pid(pid_t pid)
{
    lock_acquire(&pid_pool.pid_lock);
    int32_t bit_idx = pid - pid_pool.pid_start;
    bitmap_set(&pid_pool.pid_bitmap, bit_idx, 0);
    lock_release(&pid_pool.pid_lock);
}

pid_t fork_pid(void)
{
    return allocate_pid();
}

/* 回收thread_over的pcb和页表，归还pid, 并将其从调度队列中去除 */
void thread_exit(struct task_struct *thread_over, bool need_schedule)
{
    /* 要保证schedule在关中断情况下调用 */
    intr_disable();
    thread_over->status = TASK_DIED;

    /* 如果thread_over不是当前线程, 就有可能还在就绪队列中, 将其从中删除 */
    if (elem_find(&thread_ready_list, &thread_over->general_tag))
    {
        list_remove(&thread_over->general_tag);
    }
    if (thread_over->pgdir)
    { // 如是进程, 回收进程的页表
        mfree_page(PF_KERNEL, thread_over->pgdir, 1);
    }

    /* 从all_thread_list中去掉此任务 */
    list_remove(&thread_over->all_list_tag);

    /* 回收pcb所在的页, 主线程的pcb不在堆中, 跨过 */
    if (thread_over != main_thread)
    {
        mfree_page(PF_KERNEL, thread_over, 1);
    }

    /* 归还pid */
    release_pid(thread_over->pid);

    /* 如果需要下一轮调度则主动调用schedule */
    if (need_schedule)
    {
        schedule();
        PANIC("thread_exit: should not be here\n");
    }
}

/* 比对任务的pid */
static bool pid_check(struct list_elem *pelem, int32_t pid)
{
    struct task_struct *pthread = elem2entry(struct task_struct, all_list_tag, pelem);
    if (pthread->pid == pid)
    {
        return true;
    }
    return false;
}

/* 根据pid找pcb, 找到则返回该pcb, 否则返回NULL */
struct task_struct *pid2thread(int32_t pid)
{
    struct list_elem *pelem = list_traversal(&thread_all_list, pid_check, pid);
    if (pelem == NULL)
    {
        return NULL;
    }
    struct task_struct *thread = elem2entry(struct task_struct, all_list_tag, pelem);
    return thread;
}

/*初始化线程基本信息*/
void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    pthread->pid = allocate_pid();
    strcpy(pthread->name, name);

    if (pthread == main_thread)
    {
        /*由于把main函数也封装成一个线程， 并且它一直是运行的，所以直接设为TASK_RUNNING*/
        /* 主线程其实就是mbr->loader->kernel*/
        pthread->status = TASK_RUNNING;
    }
    else
    {
        pthread->status = TASK_READY;
    }
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE); // 线程自己在0特权级的栈，初始化在栈顶

    /* 预留标准输入输出 */
    pthread->fd_table[0] = 0;
    pthread->fd_table[1] = 1;
    pthread->fd_table[2] = 2;
    /* 其余的全置为-1 */
    uint8_t fd_idx = 3;
    while (fd_idx < MAX_FILES_OPEN_PER_PROC)
    {
        pthread->fd_table[fd_idx] = -1;
        fd_idx++;
    }

    pthread->cwd_inode_nr = 0;
    pthread->stack_magic = 0x19870916; // 自定义的魔数, 没有什么道理可言
}

/*创建一优先级为prio的线程，线程名为name, 线程所执行的函数是function(func_arg)*/
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_arg)
{
    /* pcb都位于内核空间， 包括用户进程的pcb也是在内核空间*/
    struct task_struct *thread = get_kernel_pages(1); // 返回虚拟地址
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    /*确保之前不在队列中*/
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    /*加入就绪线程队列*/
    list_append(&thread_ready_list, &thread->general_tag);

    /*确保之前不在队列中*/
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    // 使之前初始化的0弹入到对应的寄存器
    // asm volatile ("movl %0, %%esp\n pop %%ebp\n pop %%ebx\n pop %%edi\n pop %%esi\n ret" : : "g" (thread->self_kstack) : "memory");
    return thread;
}

/*将kernel中的main函数完善为主线程*/
static void make_main_thread(void)
{
    main_thread = running_thread(); // 各个线程用的0级栈都在自己的pcb中
    init_thread(main_thread, "main", 31);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

/*实现任务调度*/
void schedule()
{
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct *cur = running_thread();
    if (cur->status == TASK_RUNNING)
    {
        // 仅仅是时间片到了
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }
    else
    {
        // 阻塞
    }
    if (list_empty(&thread_ready_list))
    {
        thread_unblock(idle_thread);
    }
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct *next = elem2entry(struct task_struct,
                                          general_tag, thread_tag);
    next->status = TASK_RUNNING;
    /*激活任务页*/
    process_activate(next);
    switch_to(cur, next);
}

/*阻塞线程*/
void thread_block(enum task_status stat)
{
    ASSERT(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING)));
    /*返回原来状态并关闭中断*/
    enum intr_status old_status = intr_disable();
    struct task_struct *cur_thread = running_thread();
    cur_thread->status = stat; // 置其状态为stat
    schedule();                // 将当前线程换下处理器
    /*待当前线程被解除阻塞后才能继续运行下面得intr_set_status*/
    intr_set_status(old_status);
}

/*将线程pthread解除阻塞, 将线程加入就绪队列*/
void thread_unblock(struct task_struct *pthread)
{
    enum intr_status old_status = intr_disable();
    ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
    if (pthread->status != TASK_READY)
    {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if (elem_find(&thread_ready_list, &pthread->general_tag))
        {
            PANIC("thread_unblock: blocked thread in ready_list\n");
        }
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}

/* 主动让出cpu, 换其他线程运行 */
void thread_yield(void)
{
    struct task_struct *cur = running_thread();
    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    list_append(&thread_ready_list, &cur->general_tag);
    cur->status = TASK_READY;
    schedule();
    intr_set_status(old_status);
}

/* 将ptr输出到buf,空缺用空格填充 */
static void pad_print(char *buf, int32_t buf_len, void *ptr, char format)
{
    memset(buf, 0, buf_len);
    uint8_t out_pad_0idx = 0;
    switch (format)
    {
    case 's':
        out_pad_0idx = sprintf(buf, "%s", ptr);
        break;
    case 'd':
        out_pad_0idx = sprintf(buf, "%d", *((int16_t *)ptr));
        break;
    case 'x':
        out_pad_0idx = sprintf(buf, "%x", *((uint32_t *)ptr));
    }
    while (out_pad_0idx < buf_len)
    { // 以空格填充
        buf[out_pad_0idx] = ' ';
        out_pad_0idx++;
    }
    sys_write(stdout_no, buf, buf_len - 1);
}

/* 打印线程/进程信息，用于在list_traversal函数中的回调函数, 用于针对线程队列的处理 */
static bool elem2thread_info(struct list_elem *pelem, int arg UNUSED)
{
    struct task_struct *pthread = elem2entry(struct task_struct, all_list_tag, pelem);
    char out_pad[16] = {0};

    pad_print(out_pad, 16, &pthread->pid, 'd');

    if (pthread->parent_pid == -1)
    {
        pad_print(out_pad, 16, "NULL", 's');
    }
    else
    {
        pad_print(out_pad, 16, &pthread->parent_pid, 'd');
    }

    switch (pthread->status)
    {
    case 0:
        pad_print(out_pad, 16, "RUNNING", 's');
        break;
    case 1:
        pad_print(out_pad, 16, "READY", 's');
        break;
    case 2:
        pad_print(out_pad, 16, "BLOCKED", 's');
        break;
    case 3:
        pad_print(out_pad, 16, "WAITING", 's');
        break;
    case 4:
        pad_print(out_pad, 16, "HANGING", 's');
        break;
    case 5:
        pad_print(out_pad, 16, "DIED", 's');
    }
    pad_print(out_pad, 16, &pthread->elapsed_ticks, 'x');

    memset(out_pad, 0, 16);
    ASSERT(strlen(pthread->name) < 17);
    memcpy(out_pad, pthread->name, strlen(pthread->name));
    strcat(out_pad, "\n");
    sys_write(stdout_no, out_pad, strlen(out_pad));
    return false; // 此处返回false是为了迎合主调函数list_traversal, 只有回调函数返回false时才会继续调用此函数
}

/* 打印任务列表 */
void sys_ps(void)
{
    char *ps_title = "PID             PPID            STAT            TICKS            COMMAND\n";
    sys_write(stdout_no, ps_title, strlen(ps_title));
    list_traversal(&thread_all_list, elem2thread_info, 0);
}

/*初始化线程环境*/
void thread_init(void)
{
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    pid_pool_init();
    process_execute(init, "init");
    // 将当前main函数创建为主线程
    make_main_thread();
    idle_thread = thread_start("idle", 10, idle, NULL);
    put_str("thread_init done\n");
}
