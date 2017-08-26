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
    pthread_mutex_init(fifoLock, NULL); //初始化互斥锁
}
Fifo::~Fifo(){

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
    int size=dataRemain[writeIndex];  //缓冲区中有多少数据
    if(size>0){    //如果缓冲区中已经有数据
        return 0;
    }
    else{
        *buff=data[writeIndex];
        return size;
    }
}
bool Fifo::ReadDone(){
    pthread_mutex_lock(fifoLock);
    dataRemain[readIndex]=0; //标志该缓冲区为空
    pthread_mutex_unlock(fifoLock);
    readIndex++;
    readIndex%=NrBuf;

}
bool Fifo::WriteDone(int num){
    pthread_mutex_lock(fifoLock);
    dataRemain[writeIndex]=num; //标志该缓冲区为空
    pthread_mutex_unlock(fifoLock);
    writeIndex++;
    writeIndex%=NrBuf;
}
//读fifo
int Fifo::ReadFifo(unsigned char *dst, int length, int off) {
    if(length<=0){
        return 0;
    }
    int size=0;  //缓冲区中有多少数据
    int cpy_length=0;
    int already_read=0;
    while(already_read<length){
        size=dataRemain[readIndex];
        if(size==0){
            usleep(10000);
            continue;
        }
        unsigned char * src=data[readIndex];
        cpy_length=(length>size)?size:length;   //选最小的那个
        memcpy(dst+off,src,cpy_length);
        pthread_mutex_lock(fifoLock);
        dataRemain[readIndex]=size-cpy_length; //标志该缓冲区为空
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
    temp=*dst;//缓冲区指针交换，为了节省时间
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
    memset(&bt_in, 0, sizeof(bt_in));
    bt_in.ep = endPointIn;  //endpoint (received from Java)
    bt_in.timeout = 0;    //  timeout in ms
    bt_in.len = LENGTH;      //      length of data

    int bufferIndex = 0;

    while(isreading) {
        if(InbuffLength[bufferIndex]==0){
            read_buffer=InBuff[bufferIndex];
            bt_in.data = read_buffer;        //the data
            int actual_read = ioctl(fd, USBDEVFS_BULK, &bt_in); //发送读USB命令给linux内核

            if (actual_read > 2) {  //读到了数据
//                LOGE("读到数据%d",actual_read);
                pthread_mutex_lock(readLock);
                InbuffLength[bufferIndex]=actual_read;    //存储待预处理的数据长度
                pthread_mutex_unlock(readLock);

                ++bufferIndex;  //指向下一个接收缓冲区
                bufferIndex %= NrBuf;
            } else {    //没有读到数据
//            LOGE("没读到，sleep");
                //usleep(1000);
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
    LOGE("进入读线程");
    d2cc->ProcLoop();
    pthread_exit(0);
}
void D2cc::ProcLoop(){
    int inbufferIndex = 0;
    int procbufferIndex = 0;
    LOGE("进入预处理线程");
    while(isreading){
        if(InbuffLength[inbufferIndex]>0&&ProcbuffLength[procbufferIndex]==0){  //如果缓冲区中有待处理的数据
            proc_buffer=InBuff[inbufferIndex];
            proc_buff_length=InbuffLength[inbufferIndex];
            //这里检查边界

            extractReadData(inbufferIndex,procbufferIndex);      //预处理一个数据包

            pthread_mutex_lock(readLock);
            InbuffLength[inbufferIndex]=0;    //存储待预处理的数据长度
            pthread_mutex_unlock(readLock);

            ++inbufferIndex;  //指向下一个接收缓冲区
            inbufferIndex %= NrBuf;
            ++procbufferIndex;  //指向下一个预处理缓冲区
            procbufferIndex %= NrBuf;
        }
        else{
            usleep(100);
        }
    }
}

//对接收到的USB原始数据进行预处理，去除时钟超时等的信息，还原USB数据
//预处理512个数据为一个单位
bool D2cc::extractReadData(int readindex,int procindex){

    int totalData = 0;

    int var17 = InbuffLength[readindex];   //USB接收缓冲区的长度

    if(var17 > 0) {
        int var18 = var17 / MaxPacketSize + (var17 % MaxPacketSize > 0?1:0);

        out_pos=ProcBuff[procindex];
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
        pthread_mutex_lock(procLock);
        ProcbuffLength[procindex]=totalData;
        pthread_mutex_unlock(procLock);
        data_available+=totalData;
        return true;

    }

}

void D2cc::OpenDevice(int fdesc,int epIn,int epOut){
    fd=fdesc;
    endPointIn=epIn;
    endPointout=epOut;

    ReadThread=new pthread_t;
    ProcThread=new pthread_t;
    readLock=new pthread_mutex_t;
    procLock=new pthread_mutex_t;
    pthread_mutex_init(readLock, NULL); //初始化互斥锁
    pthread_mutex_init(procLock, NULL);
    int result1 = pthread_create(ReadThread, NULL,ReadThreadFun, (void*)this); //创建USB读取线程
    int result2 = pthread_create(ProcThread, NULL, ProcThreadFun, (void*)this); //创建数据预处理线程
//    LOGE("打开成功");
    isOpen=1;

}

void D2cc::CloseDevice(){
    if(isOpen){
        fd=0;
        endPointIn=0;
        endPointout=0;
    }
}

int D2cc::GetAvailable(){

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
    if(length<=0){
        return 0;
    }
    int size=0;  //缓冲区中有多少数据
    int cpy_length=0;
    int already_read=0;
    while(already_read<length){
        size=ProcbuffLength[read_pos];
        if(size==0){
            usleep(10000);
            continue;
        }
        unsigned char * src=ProcBuff[read_pos];
        cpy_length=(length>size)?size:length;   //选最小的那个
        memcpy(dst+off,src,cpy_length);
        pthread_mutex_lock(procLock);
        ProcbuffLength[read_pos]=size-cpy_length; //标志该缓冲区为空
        already_read+=cpy_length;
        pthread_mutex_unlock(procLock);
        if(size==cpy_length){   //一行缓冲区读完
            read_pos++;
            read_pos%=NrBuf;
        }
        else{
            memcpy(src,(src+cpy_length),size-cpy_length);
        }
    }
    return already_read;
}

/*
 * 整块读取数据（一块大概4096*4byte）
 * @bulk:块数
 * */
int D2cc::BulkRead(unsigned char *dst, int bulk) {
    int already_read=0;
    for(int i=0;i<bulk;i++){
        int size=ProcbuffLength[read_pos];  //缓冲区中有多少数据

        if(size>0){
            unsigned char * src=ProcBuff[read_pos];
            memcpy(dst+already_read,src,size);
            pthread_mutex_lock(procLock);
            ProcbuffLength[read_pos]=0; //标志该缓冲区为空
            pthread_mutex_unlock(procLock);

            already_read+=size;
            data_available-=size;
            read_pos++;
            read_pos%=NrBuf;
        }
        else{
            return already_read;
        }

    }
    return already_read;
}

extern "C" {
    JNIEXPORT void JNICALL
    Java_ir_bigandsmall_hiddevice_D2ccDevice_OpenDevice(JNIEnv *env, jobject thiz, jint jfdesc,
                                                        jint jEndPointIn, jint jEndPointOut) {
        D2cc::getInstance()->OpenDevice(jfdesc, jEndPointIn, jEndPointOut);
        //        LOGI("文件描述符%d",fdesc);
    }
    JNIEXPORT void JNICALL
    Java_ir_bigandsmall_hiddevice_D2ccDevice_CloseDevice(JNIEnv *env, jobject instance) {
        D2cc::getInstance()->CloseDevice();
    }
}