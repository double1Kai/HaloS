#ifndef HALOS_IDE_H
#define HALOS_IDE_H

#include <halos/types.h>
#include <halos/mutex.h>

#define SECTOR_SIZE 512 //扇区大小

#define IDE_CTRL_NR 2 //控制器数量，固定为2
#define IDE_DISK_NR 2 //每个控制器可挂磁盘数量，也固定为2

//IDE磁盘
typedef struct ide_disk_t{
    char name[8];           //磁盘名称
    struct ide_ctrl_t *ctrl;//控制器指针
    u8 selector;            //磁盘选择
    bool master;            //主盘
} ide_disk_t;

//IDE控制器
typedef struct ide_ctrl_t{
    char name[8];           //控制器名称
    lock_t lock;            //控制器锁
    u16 iobase;             //IO寄存器基址
    ide_disk_t disks[IDE_DISK_NR];//磁盘
    ide_disk_t *active;     //当前选择的磁盘
} ide_ctrl_t;

//读磁盘，读到buf里，读count的扇区，扇区从lba开始
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba);
//写磁盘，从buf开始写，写count的扇区，扇区从lba开始
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, idx_t lba);

#endif