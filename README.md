# 自己实现TCP Socket

Socket对程序来说与文件或输入输出流类似。TCP确保了数据的正确传输。

## lab1：
to create a reliable byte-stream out of not-so-reliable datagrams.
## lab2：
首先绝对seg序列和相对seg序列号的转换。
unwrap即从TCP segNo.转换成uint64的绝对序列比较难，要考虑checkpoint，需要熟悉uint数减法与int的关系。
uint32_t a,b;
uint32 c=a-b;//a<b时得到的值是2^32-(b-a).
int32 d=a-b;//a<b时 static_cast<int32_t>(c)==d

注意要考虑到各种情形：

* 高序列的段落先到时不会进行合并，ackno也不会变化，但会保存这些内容，一旦中间缺失的段落补齐就会assemble之前到达的段落，同时ackno也要变化。
* 尤其是reassembler类要考虑的情形非常多，TCPsegment乱序到达，需要进行整合，同时还要顾及capacity。如果高序列a先到达保存，较低序列b后到达，但此时capacity已经不够，就用b覆盖a在capacity中位置相重合的部分，同时更新fin为a的fin。

## lab3
注意当数据类型是unsigned的时候，不要通过相减结果大于0判断大小，因为两个unsigned数相减不会得到负数。
当接收窗口为0时情况略复杂，如果接收窗口为0且nextseqno==窗口右侧的seqno（windowedge），则认为是zerowindow状况，这时会认为另一端窗口为1并构建一个segment发送过去。这一操作叫做0窗口检测，是为了及时获取窗口尺寸，如果一旦窗口为0就不发送segment，那么当窗口不再为0时是不会收到通知的，因为只有发过去segment，才会传回ack和windowsize

## lab4
TCP connection处理传来的segment，如果connection还alive，把相应的信息分别给sender和receiver，然后把receiver更新的ackno和windowsize添加到sender给的seg上，并把最终的seg添加到connection的segments队列里等待传输。

* unclean end：直接调用析构函数，把sender和receiver的stream都设置为error状态后，发送包含reset flag的segment给对方。

* clean end：sender和receiver都eof：
  * A：如果receiver早于sender结束，则一定会反馈ackno给对方，此时对方sender已经结束，对方还会返回ack的ack，等到我方sender的所有消息都被对方接收，即没有byte in flight而且没有new ackno，此时对方、我方都可以结束，alive转为false。
  * B: 如果receiver晚于sender结束，无法确认对方是否知道已经被ack，因为不会收到ack的ack。因此需要等待2分钟，判断对方是否会重发segments，判断是否可以结束连接 。
    如果对方2分钟不发消息则结束。

## lab5:

tcp in ip in Ethernet： 
Network Interface处理
* 传来的Ethernet frame：
是IPv4：提取payload（Internet Datagram）给网络层
是ARP：更新表。返回一个arp_frame。
* 要发送的Internet Datagram
封装Ethernet头（MAC地址），发送。

