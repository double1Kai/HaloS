# 内核加载器loader
## 内存检测
操作系统需要直到当前运行的机器的内存分布情况  
0x15系统调用的子功能0xe820提供了这一功能  
- EAX：0xe820
- EBX：第一次调用时设置为0，表示下一个待返回的内存ARDS
- ES:DI：ARDS缓冲区
- ECX：ARDS字节大小，20
- EDX：固定签名0x534d4150，是SMAP的ASCII码，用于校验

调用后
- CF：0->未出错，1->出错
- EAX：固定签名0x534d4150
- EBX：在CF为0的前提现，EBX也为0则表示结束

返回的数据结构是Address Range Descriptor Structure，ARDS  
|字节偏移量|属性名称|描述|
|--|--|--|
|0|BaseAddrLow|基地址低32位|
|4|BaseAddrHigh|基地址高32位|
|8|LengthLow|内存长度低32位，单位是字节|
|12|LengthHigh|内存长度高32位，单位是字节|
|16|Type|本段内存的类型|
Type：
- 1->本段内存可以被操作系统使用
- 2->本段内存不可以被操作系统使用
- 其他->未定义
