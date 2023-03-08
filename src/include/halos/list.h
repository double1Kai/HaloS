#ifndef HALOS_LIST_H
#define HALOS_LIST_H

#include <halos/types.h>

#define element_offset(type, member) (u32)(&((type *)0)->member)

//通过node的指针求到包含该node的结构体的指针
#define element_entry(type, member, ptr) (type *)((u32)ptr - element_offset(type, member))

//获取node对应任务的key值
#define element_node_key(node, offset) *(int *)((int)node + offset)

//获取node到key的偏移量
#define element_node_offset(type, node, key) ((int)(&((type *)0)->key) - (int)(&((type *)0)->node))

//链表节点，没有数据段，可以用特殊方法将同一块链接到多个不同链表中
typedef struct list_node_t
{
    struct list_node_t *prev;
    struct list_node_t *next;
} list_node_t;

//链表
typedef struct list_t{
    list_node_t head;
    list_node_t tail;
} list_t;

//初始化链表
void list_init(list_t *list);
//在anchor节点前插入node
void list_insert_before(list_node_t *anchor, list_node_t *node);
//在anchor节点后插入node
void list_insert_after(list_node_t *anchor, list_node_t *node);
//插入到头结点后
void list_push(list_t *list, list_node_t *node);
//移除头结点后的节点
list_node_t *list_pop(list_t *list);
//插入到尾节点前
void list_pushback(list_t *list, list_node_t *node);
//移除尾节点前的节点
list_node_t *list_popback(list_t *list);
//查找链表中节点是否存在
bool list_search(list_t *list, list_node_t* node);
//从链表中删除节点
void list_remove(list_node_t *node);
//判断链表是否为空
bool list_empty(list_t *list);
//获取链表长度
u32 list_size(list_t *list);
//链表插入排序
void list_insert_sort(list_t *list, list_node_t *node, int offset);
 
#endif