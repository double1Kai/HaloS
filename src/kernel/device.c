#include <halos/device.h>
#include <halos/string.h>
#include <halos/task.h>
#include <halos/assert.h>
#include <halos/debug.h>
#include <halos/arena.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

//设备数量
#define DEVICE_NR 64

static device_t devices[DEVICE_NR];

//获取空设备
static device_t *get_null_device(){
    //从1开始，因为0就是空设备
    for (size_t i = 1; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        if(device->type == DEV_NULL){
            return device;
        }
    }
    panic("no more devices!!!\n");
}

//安装设备
dev_t device_install(
    int type, int subtype,
    void *ptr, char *name, dev_t parent,
    void *ioctl, void *read, void *write    
)
{
    //先获取一个空设备
    device_t *device = get_null_device();
    device->ptr = ptr;
    device->parent = parent;
    device->type = type;
    device->subtype = subtype;
    strncpy(device->name, name, NAMELEN);
    device->ioctl = ioctl;
    device->read = read;
    device->wrtie = write;
    return device->dev;

}

//根据子类型查找设备，idx表示是该子类型的第多少个
device_t *device_find(int subtype, idx_t idx){
    idx_t nr = 0;
    for (size_t i = 0; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        if(device->subtype != subtype){
            continue;
        }
        if (nr == idx){
            return device;
        }
        nr++;
    }
    return NULL;
}

//根据设备号查找设备
device_t *device_get(dev_t dev){
    assert(dev < DEVICE_NR);
    device_t *device = &devices[dev];
    assert(device->type != DEV_NULL);
    return device;
}

//控制设备
int device_ioctl(dev_t dev, int cmd, void *args, int flags){
    device_t *device =  device_get(dev);
    if(device->ioctl){
        return device->ioctl(device->ptr, cmd, args, flags);
    }
    LOGK("ioctl of device %d not implemented!!!\n", dev);
    return EOF;
}

//读设备
int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags){
    device_t *device =  device_get(dev);
    if(device->read){
        return device->read(device->ptr, buf, count, idx, flags);
    }
    LOGK("read of device %d not implemented!!!\n", dev);
    return EOF;
}

//写设备
int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags){
    device_t *device =  device_get(dev);
    if(device->wrtie){
        return device->wrtie(device->ptr, buf, count, idx, flags);
    }
    LOGK("write of device %d not implemented!!!\n", dev);
    return EOF;
}

//执行块设备请求
static void do_request(request_t *req){
    switch (req->type)
    {
    case REQ_READ:
        device_read(req->dev, req->buf, req->count, req->idx, req->flags);
        break;
    case REQ_WRITE:
        device_write(req->dev, req->buf, req->count, req->idx, req->flags);
        break;
    default:
        panic("unknown req type!!!\n");
        break;
    }
}

//获取下一个请求
static request_t *request_nextreq(device_t *device, request_t *req){
    list_t *list = &device->request_list;
    if (device->direct == DIRECT_UP && req->node.next == &list->tail)
    {   
        //到顶了，翻转方向
        device->direct = DIRECT_DOWN;
    }
    else if (device->direct == DIRECT_DOWN && req->node.prev == &list->head)
    {   
        //到底了，翻转方向
        device->direct = DIRECT_UP;
    }

    void *next = NULL;
    if(device->direct == DIRECT_UP){
        next = req->node.next;
    }else{
        next = req->node.prev;
    }

    if (next == &list->head || next == &list->tail)
    {
        return NULL;
    }
    
    return element_entry(request_t, node, next);
}

//块设备请求
void device_request(dev_t dev, void *buf, u8 count, idx_t idx, int flags, u32 type){
    device_t *device = device_get(dev);
    assert(device->type = DEV_BLOCK);
    //得到扇区位置
    idx_t offset = idx + device_ioctl(device->dev, DEV_CMD_SECTOR_START, 0, 0);
    //如果是存在辐父设备(是分区)，找到他的父设备(磁盘)
    if(device->parent){
        device = device_get(device->parent);
    }

    request_t *req = kmalloc(sizeof(request_t));

    req->dev = dev;
    req->buf = buf;
    req->count = count;
    req->idx = offset;
    req->flags = flags;
    req->type = type;
    req->task = NULL;

    LOGK("dev %d request idx %d\n", req->dev, req->idx);

    //判断是否需要排队
    bool empty = list_empty(&device->request_list);
    
    //将请求插入链表
    list_insert_sort(&device->request_list, &req->node, element_node_offset(request_t, node, idx));

    //如果链表不为空，则需要阻塞
    if(!empty){
        req->task = running_task();
        task_block(req->task, NULL, TASK_BLOCKED);
    }

    //若链表为空，或者被唤醒，执行请求
    do_request(req);
    request_t *nextreq = request_nextreq(device, req);

    //移除node
    list_remove(&req->node);
    
    kfree(req);

    //若下一个请求存在，唤醒对应的任务
    if (nextreq)
    {
        assert(nextreq->task->magic == HALOS_MAGIC);
        task_unblock(nextreq->task);
    }
}

void device_init(){
    for (size_t i = 0; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        strcpy((char *)device->name, "null");
        device->type = DEV_NULL;
        device->subtype = DEV_NULL;
        device->dev = i;
        device->parent = 0;
        device->ioctl = NULL;
        device->read = NULL;
        device->wrtie = NULL;
        list_init(&device->request_list);
        device->direct = DIRECT_UP;//初始方向为上
    }
}