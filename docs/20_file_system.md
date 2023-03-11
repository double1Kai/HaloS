## 高速缓冲
性能不同的系统之间应该存在缓冲

文件系统会以块为单位访问磁盘，块一般是2^n个扇区，其中4K一块比较常见，但是，我选择2个扇区一个块，也就是1K
高速缓冲将块存放在哈希表中，降低对磁盘的访问频率，写回法

hash表中的算缓存，data中的算缓冲

## 文件系统
参考（直接装）minix的文件系统，之后也方便抄
创建文件系统，第一版minix，文件名长度14字节，在loop0设备的第一个分区
```sudo mkfs.minix -1 -n 14 /dev/loop0p1```

文件系统就是要记录哪个文件放在哪个块里，常见的文件系统分为
- 文件分配表File Allocation Table FAT
- 索引表

FAT：有一个inode来记录这些块存在什么地方，套娃
```c++
typedef struct inode_desc_t
{
    u16 mode;       //文件类型和属性
    u16 uid;        //用户id（文件拥有者的标识符）
    u32 size;       //文件大小，以字节为单位
    u32 mtime;      //修改时间戳，使用UCT时间
    u8 gid;         //组id，文件拥有者所在的组
    u8 nlinks;      //链接数，即有多少个文件目录指向该节点
    u16 zone[9];    //直接0~6，间接7，双重间接8，逻辑块号，u16意味着每次间接能多512个块
} inode_desc_t;
```
将磁盘分成两部分，一部分存储inode，一部分存储文件块
用超级块来记录inode等信息，第0块是主引导扇区所在的块，inode就放在第1块
描述符是在磁盘里的，内存中还需要额外的结构来存储
```c++
typedef struct super_desc_t
{
    u16 inodes;     //有多少个inode
    u16 zones;      //有多少个文件块
    u16 imap_blocks;//inode的位图占用了多少块
    u16 zmap_blocks;//块的位图占用了多少块
    u16 firstdatazone;  //第一个数据逻辑块号，数据块的起始
    u16 log_zone_size;  //log2(文件块数)，一个zone占据多少块
    u32 max_size;       //文件最大长度，7+64+64*64 K
    u16 magic;          //文件系统魔数
} super_desc_t;
```

目录当做文件来看，如果一个文件是目录的话，那么inode的文件块中就存储了这样的结构
```c++
typedef struct dentry_t{
    u16 nr; //inode的索引
    char name[NAMELEN]; //文件名
} dentry_t;
```