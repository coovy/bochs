#include "list.h"
#include "interrupt.h"

/*初始化双向链表*/
void list_init(struct list* list){
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.next = NULL;
    list->tail.prev = &list->head;
}
void list_insert_before(struct list_elem *before, struct list_elem *elem){
    enum intr_status old_status = intr_disable();
    /*将before的前驱的后继更改为elem，暂时使得before脱离链表*/
    before->prev->next = elem;

    elem->prev = before->prev;
    elem->next = before;

    before->prev = elem;

    intr_set_status(old_status);
}

/*添加元素到列表队首，类似栈push操作*/
void list_push(struct list* plist, struct list_elem* elem){
    list_insert_before(plist->head.next, elem);
}

void list_append(struct list* plist, struct list_elem* elem){
    list_insert_before(&plist->tail, elem);
}

void list_remove(struct list_elem* pelem){
    enum intr_status old_status = intr_disable();

    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;

    intr_set_status(old_status);
}

/*弹出链表第一个元素*/
struct list_elem* list_pop(struct list* plist){
    struct list_elem *elem = plist->head.next;
    list_remove(elem);
    return elem;
}

/*查找obj_elem, 成功返回true，失败返回false*/
bool elem_find(struct list* plist, struct list_elem* obj_elem){
    struct list_elem *elem = plist->head.next;
    while(elem != &plist->tail){
        if(elem == obj_elem){
            return true;
        }
        elem = elem->next;
    }
    return false;
}

/*把列表plist中的每个元素elem和arg传给回调函数func,
arg给func用来判断elem是否符合条件。
本函数的功能是遍历所有元素，逐个判断是否有符合条件的元素。
找到符合条件的元素返回元素指针(仅仅返回第一个)，否则返回NULL。
*/
struct list_elem* list_traversal(struct list* plist, function func, int arg){
    struct list_elem *elem = plist->head.next;
    /*如果队列为空, 就必然没有符合条件的结点, 故直接返回NULL*/
    if(list_empty(plist)){
        return NULL;
    }
    while(elem != &plist->tail){
        if(func(elem, arg)){
            // func返回true, 则认为该元素在回调函数中符合条件，命中，故停止继续遍布
            return elem;
        }
        elem = elem->next;
    }
    return NULL;
}

/*返回链表长度*/
uint32_t list_len(struct list* plist){
    struct list_elem *elem = plist->head.next;
    uint32_t length = 0;
    while(elem != &plist->tail){
        length++;
        elem = elem->next;
    }
    return length;
}

bool list_empty(struct list* plist){
    return (plist->head.next == &plist->tail ? true : false);
}