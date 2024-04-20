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

struct lock pid_lock;

/*分配pid*/
static pid_t allocate_pid(void){
    static pid_t next_pid = 0;      // 静态变量
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}

struct task_struct* idle_thread;    // idle线程, 在系统中没有其他任务需要运行时占用CPU
struct lock pid_lock;		    // 分配pid锁
struct task_struct *main_thread;       // 主线程pcb
struct list thread_ready_list;  // 就绪队列
struct list thread_all_list;    // 所有任务队列
static struct list_elem *thread_tag; //用于保存队列中的线程结点,
                                    //  队列中的结点不是pcb，需要将tag转换为pcb，所以记录tag用
extern void switch_to(struct task_struct *cur, struct task_struct *next);


/* 系统空闲时运行的线程 */
static void idle(void* arg UNUSED){
    while(1){
        thread_block(TASK_BLOCKED);
        // 执行hlt时必须要保证目前处在开中断的情况下
        asm volatile("sti\n hlt" : : : "memory");   // hlt在系统中没有其他任务需要运行时占用CPU
    }
}


/*获取当前pcb指针(虚拟地址)*/
struct task_struct* running_thread(){
    uint32_t esp;
    asm("mov %%esp, %0" : "=g"(esp));
    /*取esp整数部分, 即pcb起始地址*/
    return (struct task_struct *)(esp & 0xfffff000);
}

/* 由kernel_thread去执行functionion(funct_arg)*/
static void kernel_thread(thread_func* function, void* func_arg){
    intr_enable();  // 避免时钟中断被屏蔽
    function(func_arg);
}

/*初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack相应的位置*/
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg){
    /*先预留中断使用栈的空间，可见thread.h定义的结构*/
    pthread->self_kstack -= sizeof(struct intr_stack);

    /*再留出线程栈空间, 可见thread.h定义*/
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;// 这四个寄存器执行过程才会有值
}

/*初始化线程基本信息*/
void init_thread(struct task_struct* pthread, char* name, int prio){
    memset(pthread, 0, sizeof(*pthread));
    pthread->pid = allocate_pid();
    strcpy(pthread->name, name);

    if(pthread == main_thread){
        /*由于把main函数也封装成一个线程， 并且它一直是运行的，所以直接设为TASK_RUNNING*/
        /* 主线程其实就是mbr->loader->kernel*/
        pthread->status = TASK_RUNNING;
    } else{
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
    while (fd_idx < MAX_FILES_OPEN_PER_PROC){
        pthread->fd_table[fd_idx] = -1;
        fd_idx++;
    }
    
    pthread->stack_magic = 0x19870916; // 自定义的魔数, 没有什么道理可言
}

/*创建一优先级为prio的线程，线程名为name, 线程所执行的函数是function(func_arg)*/
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg){
    /* pcb都位于内核空间， 包括用户进程的pcb也是在内核空间*/
    struct task_struct* thread = get_kernel_pages(1);   // 返回虚拟地址
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
static void make_main_thread(void){
    main_thread = running_thread();   // 各个线程用的0级栈都在自己的pcb中
    init_thread(main_thread, "main", 31);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

/*实现任务调度*/
void schedule(){
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct *cur = running_thread();
    if(cur->status == TASK_RUNNING){
        // 仅仅是时间片到了
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    } else{
        // 阻塞
    }
    if(list_empty(&thread_ready_list)){
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
void thread_block(enum task_status stat){
    ASSERT(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING)));
    /*返回原来状态并关闭中断*/
    enum intr_status old_status = intr_disable();
    struct task_struct *cur_thread = running_thread();
    cur_thread->status = stat;          // 置其状态为stat
    schedule();                         // 将当前线程换下处理器
    /*待当前线程被解除阻塞后才能继续运行下面得intr_set_status*/
    intr_set_status(old_status);
}

/*将线程pthread解除阻塞, 将线程加入就绪队列*/
void thread_unblock(struct task_struct* pthread){
    enum intr_status old_status = intr_disable();
    ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
    if(pthread->status != TASK_READY){
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if(elem_find(&thread_ready_list, &pthread->general_tag)){
            PANIC("thread_unblock: blocked thread in ready_list\n");
        }
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}

/* 主动让出cpu, 换其他线程运行 */
void thread_yield(void) {
   struct task_struct* cur = running_thread();   
   enum intr_status old_status = intr_disable();
   ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
   list_append(&thread_ready_list, &cur->general_tag);
   cur->status = TASK_READY;
   schedule();
   intr_set_status(old_status);
}

/*初始化线程环境*/
void thread_init(void){
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);
    // 将当前main函数创建为主线程
    make_main_thread();
    idle_thread = thread_start("idle", 10, idle, NULL);
    put_str("thread_init done\n");
}


