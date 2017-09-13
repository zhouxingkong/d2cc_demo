//
// Created by xingkong on 2017/4/26.
//

#include "d2cc.h"
D2cc* D2cc::mInstance;



Fifo::Fifo(){
    for(int i=0;i<NrBuf;i++){
        data[i]=new unsigned char [LENGTH];
        dataRemain[i]=0;
    }
    fifoLock=new pthread_mutex_t;
    pthread_mutex_init(fifoLock, NULL); //初始化互斥锁
}
Fifo::~Fifo(){
    for(int i=0;i<NrBuf;i++){
        delete data[i];
    }
}

int Fifo::getReadBuff(unsigned char ** buff){
    int size=dataRemain[readIndex];  //缓冲区中有多少数据
    if(size==0){    //如果缓冲区中没有数据可读取就直接返回0
        return 0;
    }
    else{
        *buff=data[readIndex];
        return size;
    }
}
int Fifo::getWriteBuff(unsigned char ** buff){
//    LOGE("进入getWriteBuff");
    int size=dataRemain[writeIndex];  //缓冲区中有多少数据
    if(size>0){    //如果缓冲区中已经有数据
        return size;
    }
    else{
        *buff=data[writeIndex];
        return 0;
    }
}
bool Fifo::ReadDone(){
    pthread_mutex_lock(fifoLock);
    dataRemain[readIndex]=0; //标志该缓冲区为空
    pthread_mutex_unlock(fifoLock);
    readIndex++;
    readIndex%=NrBuf;
    return true;

}
bool Fifo::WriteDone(int num){
    pthread_mutex_lock(fifoLock);
    dataRemain[writeIndex]=num; //标志该缓冲区为空
    pthread_mutex_unlock(fifoLock);
    writeIndex++;
    writeIndex%=NrBuf;
//    LOGE("WriteDone结束");
    return true;
}
//读fifo
int Fifo::ReadFifo(unsigned char *dst, int length, int off) {
    if(length<=0){
        return 0;
    }
    int size=0;  //缓冲区中有多少数据
    int cpy_length=0;
    int already_read=0;
    unsigned char * src;
    while(already_read<length){
        size=dataRemain[readIndex];
        if(size==0){
            usleep(10000);
            continue;
        }
        src=data[readIndex];
        cpy_length=((length-already_read)>size)?size:(length-already_read);   //选最小的那个
        memcpy(dst+off+already_read,src,cpy_length);
        pthread_mutex_lock(fifoLock);
        dataRemain[readIndex]=size-cpy_length;
        already_read+=cpy_length;
        pthread_mutex_unlock(fifoLock);
        if(size==cpy_length){   //一行缓冲区读完
            readIndex++;
            readIndex%=NrBuf;
        }
        else{
            memcpy(src,(src+cpy_length),size-cpy_length);
        }
    }
    return already_read;
}
//一次读一整块
int Fifo::ReadFifoBulk(unsigned char ** dst) {
    int size=dataRemain[readIndex];  //缓冲区中有多少数据
    if(size==0){    //如果缓冲区中没有数据可读取就直接返回0
        return 0;
    }
    pthread_mutex_lock(fifoLock);
    temp=*dst;  //缓冲区指针交换，为了节省时间
    *dst=data[readIndex];
    data[readIndex]=temp;
    dataRemain[readIndex]=0;
    pthread_mutex_unlock(fifoLock);

    readIndex++;
    readIndex%=NrBuf;
    return size;
}
//写fifo
int Fifo::WriteFifo(unsigned char *src, int length) {

}

D2cc::D2cc(){
    ReadThread=new pthread_t;
    ProcThread=new pthread_t;
    readLock=new pthread_mutex_t;
    procLock=new pthread_mutex_t;
//    proc_buffer=new unsigned char [LENGTH];
    pthread_mutex_init(readLock, NULL); //初始化互斥锁
    pthread_mutex_init(procLock, NULL);
}
D2cc::~D2cc(){

}
D2cc* D2cc::getInstance(){
    if(mInstance==NULL){
        mInstance=new D2cc;
    }
    return mInstance;
}

//读取USB读取线程
void  * D2cc::ReadThreadFun(void *pArguments){
    D2cc *d2cc=(D2cc *)pArguments;
    LOGE("进入读线程");
    d2cc->ReadLoop();
    pthread_exit(0);
}
void D2cc::ReadLoop(){
//    memset(&bt_in, 0, sizeof(bt_in));
    bt_in.ep = endPointIn;  //endpoint (received from Java)
//    LOGE("bt_in.ep=%d",bt_in.ep);
    bt_in.timeout = 1000;    //  timeout in ms
    bt_in.len = LENGTH;      //      length of data
    while(isOpen) {
        if(!isreading){
            usleep(10000);
            continue;
        }
        if(fifoRead.getWriteBuff(&read_buffer)==0){
            bt_in.data = (void *)read_buffer;        //the data
            int actual_read = ioctl(fd, USBDEVFS_BULK, &bt_in); //发送读USB命令给linux内核
//            int actual_read=0;
            if (actual_read > 8) {  //读到了数据
//                LOGE("读到数据%d",actual_read);
                fifoRead.WriteDone(actual_read);
            } else {    //没有读到数据
//            LOGE("没读到，sleep");
                usleep(1000);
                continue;
            }
        }
        else{   //如果预处理线程没有处理完数据，就不读取数据
//            LOGE("读缓冲区满了");
            usleep(100);
        }
    }

}

//读取数据预处理线程
void  * D2cc::ProcThreadFun(void *pArguments){
    D2cc *d2cc=(D2cc *)pArguments;
    d2cc->ProcLoop();
    pthread_exit(0);
}
void D2cc::ProcLoop(){
    LOGE("进入预处理线程");
    while(isOpen) {
        if(!isreading){
            usleep(10000);
            continue;
        }
        read_out_buff_length=fifoRead.getReadBuff(&proc_buffer);
        proc_in_buff_length=fifoProc.getWriteBuff(&out_pos);
        if(proc_in_buff_length>0){
            LOGE("预处理FIFO满");
        }
        if(read_out_buff_length<=0||proc_in_buff_length>0){
            usleep(1000);
            continue;
        }
//        LOGE("预处理线程检测到数据");
        int ret=extractReadData();      //预处理一个数据包
//        LOGE("一包数据预处理完成");
//        int ret=0;
        fifoRead.ReadDone();
        fifoProc.WriteDone(ret);
        data_available+=ret;

    }
}
//对接收到的USB原始数据进行预处理，去除时钟超时等的信息，还原USB数据
//预处理512个数据为一个单位
int D2cc::extractReadData(){
    int totalData = 0;
    int var17 = read_out_buff_length;   //USB接收缓冲区的长度
    int var18 = var17 / MaxPacketSize + (var17 % MaxPacketSize > 0?1:0);
    for(int ex = 0; ex < var18; ++ex) {
        int var19;
        int var20;
        if(ex == var18 - 1) {
            if(var17)
                var20 = var17;
            var19 = ex * MaxPacketSize;
            var19 += 2;
            if(var19!=var20){   //如果数据包只有2字节，就不要了
                memcpy(out_pos,proc_buffer+ex * MaxPacketSize+2,var20-var19);     //把剩余的数据都取出来
            }
        } else {
            var20 = (ex + 1) * MaxPacketSize;
            var19 = ex * MaxPacketSize + 2;
            memcpy(out_pos,proc_buffer+ex * MaxPacketSize+2,var20-var19);
        }
        totalData += var20 - var19;
        out_pos+=(var20 - var19);   //指针指向预处理缓冲区下一个位置
    }
    return totalData;
}

void D2cc::OpenDevice(int fdesc,int epIn,int epOut){
    LOGE("C++的OpenDevice函数,fd=%d",fdesc);
    fd=fdesc;
    endPointIn=epIn;
    endPointout=epOut;
    isOpen=1;
    int result1 = pthread_create(ReadThread, NULL,ReadThreadFun, (void*)this); //创建USB读取线程
    int result2 = pthread_create(ProcThread, NULL, ProcThreadFun, (void*)this); //创建数据预处理线程
//    LOGE("打开成功");


}

void D2cc::CloseDevice(){
    if(isOpen){
        fd=0;
        endPointIn=0;
        endPointout=0;
        pthread_join(*ReadThread,NULL);
        pthread_join(*ProcThread,NULL);
        isreading=0;
    }
}

int D2cc::GetAvailable(){
    return data_available;
}

int D2cc::Write(unsigned char * src,int length){
    //LOGE("正在写入数据");
    if(!isOpen){    //没有打开设备，就返回0
        return 0;
    }
    int packs=length/LENGTH+(((length%LENGTH)==0)?0:1);
    int already_write=0;
    int res=0;
    bt_out.ep = endPointout;  /* endpoint (received from Java) */
    bt_out.timeout = 1000;      /* timeout in ms */
    for(int i=0;i<packs;i++){
        res=length-already_write;
        if(i==packs-1){
            memcpy(OutBuff,src+already_write,res);
            bt_out.len = res;            /* length of data */
            bt_out.data =(void *)OutBuff;        /* the data */
            ioctl(fd, USBDEVFS_BULK, &bt_out);   //发送写USB命令给linux内核
        }
        else{
            memcpy(OutBuff,src+already_write,LENGTH);
            bt_out.len = LENGTH;            /* length of data */
            bt_out.data =(void *)OutBuff;        /* the data */
            ioctl(fd, USBDEVFS_BULK, &bt_out);   //发送写USB命令给linux内核
        }

    }
    return already_write;
}

int read_pos=0; //读取预处理缓冲区的索引
//读取预处理缓冲区中的数据
int D2cc::Read(unsigned char * dst,int length,int off){
    int ret= fifoProc.ReadFifo(dst,length,off);
    data_available-=ret;
    return ret;
}

/*
 * 整块读取数据（一块大概4096*4byte）
 * */
int D2cc::BulkRead(unsigned char *dst) {
    if(dst==NULL){  //如果没分配内存就分配一个，不然程序会闪退
        dst=new unsigned char [LENGTH];
    }
    int size=fifoProc.ReadFifoBulk(&dst);  //缓冲区中有多少数据
    if(size>0){
        data_available-=size;
    }
    return size;
}

