### 硬盘同步PIO Programed In/Out
和之前的一样，轮询
### 硬盘异步PIO
同步的检测消耗了大量的CPU资源，异步采用中断的方式
发出读写命令后进程进入阻塞状态，准备好了之后再引发中断，suoyi
### 硬盘识别
硬盘接口有4个，需要识别哪些有硬盘
使用IDENTITY命令
### 硬盘分区
为了多个操作系统共同使用硬盘资源，硬盘可以在逻辑上分为4个主分区，分区表存储在引导山区的0x1BE~0x1FD处
可以对分了区的扇区再进行扩展分区
使用fdisk命令对master进行分区，参考archlinuxwiki
使用sudo losetup /dev/loop0 --partscan ../build/master.img挂载到archlinux上，使用lsblk查看
看完后losetup -d /dev/loop0

将分区信息备份，保存为master.sfdisk，之后可以对新建的磁盘直接分区

### 虚拟设备，使用的设备与物理设备无关

### 磁盘调度的电梯算法
主要时间开销在寻道时间，尽可能减少磁头移动
**LBA和CHS的转换公式**
- CYL = LBA/(HPC * SPT)
- HEAD = (LBA % (HPC * SPT)) / SPT
- SECT = (LBA % (HPC * SPT)) % SPT + 1
- LBA = ((CYL * HPC + HEAD) * SPT) + SECT - 1

CYL：柱面
HEAD：磁头
SECT：扇区
LBA：逻辑地址
HPC：每个柱面的磁头数
SPT：每个磁道的扇区数
