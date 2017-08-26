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

    unsigned char * temp;

    int readIndex=0;
    int writeIndex=0;
public:
    Fifo();
    ~Fifo();

    //为了节省memcpy函数的调用，提高USB读取速度，fifo支持直接对缓冲区进行操作。
    int getReadBuff(unsigned char ** buff);
    int getWriteBuff(unsigned char ** buff);
    bool ReadDone();    //当直接操作缓冲区Buff读写fifo结束后记得调用这两个函数
    bool WriteDone(int num);

    int WriteFifo(unsigned char * src,int length);
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

    unsigned char InBuff[NrBuf][LENGTH];   //输入数据缓冲区
    int InbuffLength[NrBuf]; //输入数据缓冲区中的数据个数（待预处理）
    unsigned char ProcBuff[NrBuf][LENGTH];   //存储预处理后的数据
    int ProcbuffLength[NrBuf]; //输入数据缓冲区中的数据个数（待预处理）

    unsigned char OutBuff[LENGTH];  //数据发送缓冲区

    unsigned char *read_buffer;  //USB读写缓存(指向InBuff中的一行)
    unsigned char *proc_buffer;  //正在预处理的数据(指向InBuff中的一行)
    unsigned char *out_pos;  //正在预处理的数据(指向InBuff中的一行)
    int proc_buff_length = 0;

    int fd;
    int endPointIn;
    int endPointout;
    bool isOpen = 0;
    int data_available=0;

//    int actual_read = 0;  //每次读取实际读到的数
    bool isreading = 1;   //是否开始读
protected:
    D2cc();
    static D2cc *mInstance;

public:
    ~D2cc();
    static D2cc* getInstance();
    //线程--内部函数
    static void *ReadThreadFun(void *pArguments);
    static void *ProcThreadFun(void *pArguments);
    void ReadLoop();
    void ProcLoop();
    bool extractReadData(int readindex, int procindex);
    //调用接口
    void OpenDevice(int fdesc, int epIn, int epOut);    //打开设备
    void CloseDevice(); //关闭设备
    int GetAvailable(); //返回接收缓冲区中剩余数据量
    int Read(unsigned char *dst, int length,int off);   //任意长度读取
    int BulkRead(unsigned char *dst, int bulk); //整块数据读取
    int Write(unsigned char *src, int length);

};

#endif //ANDROID_NDK_USB_TEST_D2XX_H
