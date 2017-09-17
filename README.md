# d2cc


## 1介绍
文件结构：
d2cc库由以下三个文件构成：
* D2ccManager.java:USB管理类，负责调用Android系统提供的的API函数来打开和管理USB设备，申请USB访问权限。并将USB设备的文件描述符传入native空间，以便d2cc.cpp对USB接口进行操作。
* d2cc.cpp和d2cc.h: NDK层的主要文件，实现d2xx.jar库的基本功能。

d2cc库分为两个部分：
1. java程序：应用程序使用java语言来调用这个部分，作用为打开USB设备、申请USB访问权限（响应Android系统USB设备广播）。
2. C++程序：应用程序使用C++语言来调用这个部分。此部分作用为使用ioctl机制对系统底层USB驱动程序进行调用，实现高速的USB数据读写。
应用程序使用d2cc库的方法如下图所示：
![](https://github.com/zhouxingkong/d2cc_demo/raw/struct.png)
使用d2cc库需要应用程序同样工作在java和C++两个空间，并分别调用d2cc两个部分的库函数。（详细请参考d2cc_demo示例程序）

## 2使用d2cc库的步骤

1. 将D2ccManager.java、d2cc.cpp、d2cc.h三个文件导入Android工程。工程目录可参考下图：
![](https://github.com/zhouxingkong/d2cc_demo/raw/dir.png)

2. 编写CMakeList.txt用来编译C++代码。参考下图：
![](https://github.com/zhouxingkong/d2cc_demo/raw/cmake.png)

3. 编写java层程序调用D2ccManager.java中的方法来实例化D2ccManager类并打开USB设备。
``` java
♦ D2ccManager. getInstance(Context context1)//函数用来获取D2ccManager类实例来对USB设备进行操作。
♦ D2ccManager. OpenDevice()//函数遍历Android系统所有USB接口，并通过比较VID来识别目标USB设备。当找到目标设备后OpenDevice函数向Android系统申请该USB设备的访问权限。可以通过修改D2ccManager.java中如下字段来修改目标USB的VID值。(PS:FTDI厂家生产的所有USB芯片VID都为十进制的1027，此值并不需要修改)
```

4. 编写native层代码实现对USB的读写操作(例程这种为main.cpp & main.h)。Native层USB读写的接口函数如下所示
``` cpp
♦ int Read(unsigned char *dst, int length,int off);   //任意长度读取
♦ int BulkRead(unsigned char *dst, int bulk); //整块数据读取
♦ int Write(unsigned char *src, int length);
```
5. 编写JNI接口文件，在该文件中实现和第4步定义函数的接口。例程中的JNI接口文件为ClientJni.java。

## 3 API doc

D2ccManager.java

``` java
/*  获取D2ccManager类的静态实例。第一次调用系统会自动创建实例，
*   之后再调用就会返回第一次创建的实例
*   @return:静态实例mInstance
*   @context1:由Activity传来的Context对象
* */
public D2ccManager D2ccManager.getInstance(Context context1);

/*  打开USB设备：若已经存在打开的设备此函数胡就啥也不做
*   如果没有已打开的设备，函数会遍历Android系统接入的所有USB设备，对比设备的PID；VID是否符合要求
*   若符合要求就向Android系统申请该USB设备的访问权限。系统发送android.hardware.usb.action.USB_DEVICE_ATTACHED广播
*   @return:静态实例mInstance
*   @context1:由Activity传来的Context对象
* */
public void D2ccManager.OpenDevice();
```

D2cc.cpp

``` cpp
/* 获取D2cc实例
 * return:单例模式的D2xx实例
 * */
static D2cc* D2cc::getInstance();
/*
 * 关闭USB设备：暂停USB读取的进程，当再次调用OpenDevice则继续读取
 * */
void D2cc::CloseDevice(); 
/*
 * 返回接收缓冲区中剩余数据字节数
 * */
int D2cc::GetAvailable();
/* 从USB接收缓冲区中读取任意长度数据。函数会根据需要读取的数据量逐个遍历USB接收FIFO进行读取。
 * 若USB缓冲区没有足够数据读取，此函数会进入sleep状态等待，直到读取数据量达到length。
 * @return:成功接收的字节数
 * @dst:接收数据目的地址
 * @length:接收数据字节数
 * @off:数据放在dst数组的位置
 * */
int D2cc::Read(unsigned char *dst, int length,int off);   //任意长度读取
/* 接收一块USB数据，块长度为4096*4（可能比这个少一点）
 * @return:成功接收的字节数，没接收到返回0
 * @dst:接收数据目的地址
 * */
int D2cc::BulkRead(unsigned char *dst); //整块数据读取
/* 发送USB数据
 * @return:成功发送的字节数，没发送成功返回0
 * @src:发送数据的首地址
 * @length:发送数据的字节数
 * */
int D2cc::Write(unsigned char *src, int length);  //写入数据
```

