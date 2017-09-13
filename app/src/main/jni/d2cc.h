//
// Created by xingkong on 2017/4/26.
//

#ifndef ANDROID_NDK_USB_TEST_D2XX_H
#define ANDROID_NDK_USB_TEST_D2XX_H
#pragma once

#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <jni.h>
#include <stdlib.h>
#include <fcntl.h>
#include <android/log.h>

//各种系统参数宏定义
#define MaxPacketSize 512
#define LENGTH 4096*4   //一个接收缓冲区长度
#define NrBuf 16    //接收队列中缓冲区个数
#define ReadBuffSize 4096*20
#define NrOutBuff 256
#define RxTimeout = 5000


#define  LOG_TAG    "d2cc"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


class Fifo{
private:
//    const int ROWS=16;   //FIFO中缓冲区的个数
//    const int LENGTH = 16384;   //每个缓冲区的长度
    pthread_mutex_t *fifoLock;   //数据接收缓冲区锁
//    unsigned char data[NrBuf][LENGTH];
    unsigned char *data[NrBuf];
    int  dataRemain [NrBuf];  //每个缓冲区中剩余字节数

    unsigned char * temp_read;
    unsigned char * temp_write;
    int readIndex=0;
    int writeIndex=0;
public:
    Fifo();
    ~Fifo();

    //为了节省memcpy函数的调用，提高USB读取速度，fifo支持直接对缓冲区进行操作。
    //这四个函数已废弃
    int getReadBuff(unsigned char ** buff);
    int getWriteBuff(unsigned char ** buff);
    bool ReadDone();    //当直接操作缓冲区Buff读写fifo结束后记得调用这两个函数
    bool WriteDone(int num);

    int WriteFifo(unsigned char * src,int length);
    int WriteFifoBulk(unsigned char ** src,int length);
    int ReadFifo(unsigned char * dst,int length ,int off);
    int ReadFifoBulk(unsigned char ** dst);
};


class D2cc {
private:

    usbdevfs_bulktransfer bt_in;
    usbdevfs_bulktransfer bt_out;

    pthread_t *ReadThread;
    pthread_t *ProcThread;
    pthread_mutex_t *readLock;   //数据接收缓冲区锁
    pthread_mutex_t *procLock;   //预处理缓冲区锁

    Fifo fifoRead;
    Fifo fifoProc;

    unsigned char OutBuff[LENGTH];  //数据发送缓冲区

    unsigned char *read_buffer;  //USB读写缓存(指向InBuff中的一行)
    unsigned char *proc_in_buffer;  //正在预处理的数据(指向InBuff中的一行)
    unsigned char *proc_out_buffer;  //正在预处理的数据(指向InBuff中的一行)
    unsigned char *proc_out_buffer_pos;
    int read_out_buff_length = 0;
    int proc_in_buff_length=0;

    int fd;
    int endPointIn;
    int endPointout;
    bool isOpen = 0;
    int data_available=0;

//    int actual_read = 0;  //每次读取实际读到的数
    bool isreading = 1;   //是否开始读
    bool isopen=1;
public:
    D2cc();
    static D2cc *mInstance;

public:
    ~D2cc();
    //线程--内部函数，用户不能调用
    void OpenDevice(int fdesc, int epIn, int epOut);    //打开设备
    static void *ReadThreadFun(void *pArguments);
    static void *ProcThreadFun(void *pArguments);
    void ReadLoop();
    void ProcLoop();
    int extractReadData();

    //调用接口

    /* 获取D2cc实例
     * return:单例模式的D2xx实例
     * */
    static D2cc* getInstance();
    /*
     * 关闭USB设备：暂停USB读取的进程，当再次调用OpenDevice则继续读取
     * */
    void CloseDevice();
    /*
     * 返回接收缓冲区中剩余数据字节数
     * */
    int GetAvailable();
    /* 从USB接收缓冲区中读取任意长度数据。函数会根据需要读取的数据量逐个遍历USB接收FIFO进行读取。
     * 若USB缓冲区没有足够数据读取，此函数会进入sleep状态等待，直到读取数据量达到length。
     * @return:成功接收的字节数
     * @dst:接收数据目的地址
     * @length:接收数据字节数
     * @off:数据放在dst数组的位置
     * */
    int Read(unsigned char *dst, int length,int off);   //任意长度读取
    /* 接收一块USB数据，块长度为4096*4（可能比这个少一点）
     * @return:成功接收的字节数，没接收到返回0
     * @dst:接收数据目的地址
     * */
    int BulkRead(unsigned char **dst); //整块数据读取
    /* 发送USB数据
     * @return:成功发送的字节数，没发送成功返回0
     * @src:发送数据的首地址
     * @length:发送数据的字节数
     * */
    int Write(unsigned char *src, int length);  //写入数据

};

#endif //ANDROID_NDK_USB_TEST_D2XX_H
