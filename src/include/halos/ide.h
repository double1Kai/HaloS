#ifndef HALOS_IDE_H
#define HALOS_IDE_H

#include <halos/types.h>
#include <halos/mutex.h>

#define SECTOR_SIZE 512 //扇区大小

#define IDE_CTRL_NR 2 //控制器数量，固定为2
#define IDE_DISK_NR 2 //每个控制器可挂磁盘数量，也固定为2
#define IDE_PART_NR 4 //每个磁盘的分区数量，仅主分区，共4个

//主引导扇区里的16B
typedef struct part_entry_t
{
    u8 bootable;             // 引导标志
    u8 start_head;           // 分区起始磁头号
    u8 start_sector : 6;     // 分区起始扇区号
    u16 start_cylinder : 10; // 分区起始柱面号
    u8 system;               // 分区类型字节
    u8 end_head;             // 分区的结束磁头号
    u8 end_sector : 6;       // 分区结束扇区号
    u16 end_cylinder : 10;   // 分区结束柱面号
    u32 start;               // 分区起始物理扇区号 LBA
    u32 count;               // 分区占用的扇区数
} _packed part_entry_t;

//主引导扇区的内容
typedef struct boot_sector_t
{
    u8 code[446];//446B的分区表
    part_entry_t entry[4];//64B的分区表
    u16 signature;//2B的55aa
} _packed boot_sector_t;

//分区的实际信息
typedef struct ide_part_t
{
    char name[8];            // 分区名称
    struct ide_disk_t *disk; // 磁盘指针
    u32 system;              // 分区类型
    u32 start;               // 分区起始物理扇区号 LBA
    u32 count;               // 分区占用的扇区数
} ide_part_t;

//IDE磁盘
typedef struct ide_disk_t{
    char name[8];           //磁盘名称
    struct ide_ctrl_t *ctrl;//控制器指针
    u8 selector;            //磁盘选择
    bool master;            //主盘
    u32 total_lbas;         //可用扇区数量
    u32 cylinders;          //柱面数
    u32 head;               //磁头数
    u32 sectors;            //扇区数
    ide_part_t parts[IDE_PART_NR]; // 硬盘分区
} ide_disk_t;

//IDE控制器
typedef struct ide_ctrl_t{
    char name[8];           //控制器名称
    lock_t lock;            //控制器锁
    u16 iobase;             //IO寄存器基址
    ide_disk_t disks[IDE_DISK_NR];//磁盘
    ide_disk_t *active;     //当前选择的磁盘
    u8 control;             //控制字节，用于重置驱动器
    struct task_t *waiter;  //等待控制器的进程
    
} ide_ctrl_t;

//读磁盘，读到buf里，读count的扇区，扇区从lba开始
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, idx_t lba);
//写磁盘，从buf开始写，写count的扇区，扇区从lba开始
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, idx_t lba);

#endif