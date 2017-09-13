
#include "main.h"


void judge_error(int actual_read){

//    memcpy(judge_data,judge_buffer,actual_read);
//    if(judge_data[0]!=last_byte-1){
//        errors+=2;
//    }

//    for(int i=0;i<actual_read-2;i++){
////        int l1=judge_buffer[i+1];
////        int l2=judge_buffer[i];
//        if(read_buffer[i+1]!=(read_buffer[i]+1)%256){
//            errors++;
//            LOGE("%d!=%d",read_buffer[i+1],read_buffer[i]);
//        }
//    }
}
//检测数据是否丢包
void extract(){
    for(int i=0;i<extract_length-4;i++){
        if((buff1[i]==0xed)&&(buff1[i+1]==0xec)&&(buff1[i+2]==0xeb)&&(buff1[i+3]==0xea)){
            packages++;
//            LOGE("pack%d",buff1[i+4]);
            if(buff1[i+4]!=(last_index+1)%256){
//                LOGE("丢包%d",last_index+1);
                lost++;
//                LOGE("extract%d",extract_length);
            }
            last_index=buff1[i+4];
//             if(i<extract_length-5000) i+=4080;   //跳过其他字节
        }
    }
    memcpy(buff1,buff1+extract_length-4,4); //后边可能有半个包头，放到前面
    read_buffer_ptr=buff1+4;
    extract_length=4;
}

void  * ReadDeviceFun(void *pArguments){
    LOGE("进入主读线程");
    read_buffer_ptr=new unsigned char [4096*4];
    while(isreading) {

//        actual_read_main=d2cc.Read(read_buffer_ptr,4096*16,0);
        actual_read_main=d2cc.BulkRead(&read_buffer_ptr);
        if (actual_read_main > 0) {
//            LOGE("data%d", actual_read);
            allnum += actual_read_main;
            extract_length+=actual_read_main;
//            extract();
//            {   //存储离线数据
//                if ((count < 800) && begin) {
//                    write(ffd, read_buffer_ptr, actual_read_main);
//                    count++;
//                } else if (count == 800) {
//                    close(ffd);
//                    count++;
//                    LOGE("关闭文件");
//                }
//            }
//            judge_error(actual_read);
        } else {
//            LOGE("没读到，sleep");
            usleep(1000);
            continue;
        }
    }


    pthread_exit(0);
}

void  * JudgeThreadFun(void *pArguments){

    LOGE("进入了判断线程pthread");
    while(isreading) {
//        judge_error(actual_read);
        LOGE("一秒接收数据帧%d",packages);
        packages=0;
        LOGE("总共丢包%d",lost-1);
//        lost=0;
        LOGE("一秒接收字节%d",allnum);
        allnum=0;
//        LOGE("误码个数%d",errors/2);
//        errors=0;
        sleep(1);
    }
    pthread_exit(0);
}


//写USB函数很简单，只写四个字节的包头
    JNIEXPORT jint JNICALL  Java_ir_bigandsmall_hiddevice_D2ccDevice_WriteMemory  (JNIEnv *env,jobject thiz ) {
        out_buffer[0]=0xaa;
        out_buffer[1]=0xab;
        out_buffer[2]=0xac;
        out_buffer[3]=0xad;
        d2cc.Write(out_buffer,4);
        begin=1;
        return 1;
    }

    JNIEXPORT jshort JNICALL  Java_ir_bigandsmall_hiddevice_D2ccDevice_ReadMemory
            (JNIEnv *env,jclass clazz){
//        read_buffer=buff1;
//        judge_buffer=buff1;
        ReadThreadmain=new pthread_t;
        JudgeThreadmain=new pthread_t;
        //创建两个线程，一个用来读USB，一个用来判断读到的东西是不是对的
        int result1 = pthread_create(ReadThreadmain, NULL, ReadDeviceFun, (void*)NULL);
        int result2 = pthread_create(JudgeThreadmain, NULL, JudgeThreadFun, (void*)NULL);
//
        return 1;
    }

    JNIEXPORT void JNICALL
    Java_ir_bigandsmall_hiddevice_D2ccDevice_ArrayTest(JNIEnv *env, jobject instance, jbyteArray in_,
                                                       jbyteArray out_) {
        jbyte *in = env->GetByteArrayElements(in_, NULL);
        jbyte *out = env->GetByteArrayElements(out_, NULL);

        out[0]=in[0];
        // TODO

        env->ReleaseByteArrayElements(in_, in, 0);
        env->ReleaseByteArrayElements(out_, out, 0);
    }

    JNIEXPORT void JNICALL
    Java_ir_bigandsmall_hiddevice_D2ccDevice_OpenDevice(JNIEnv *env, jobject thiz, jint jfdesc,
                                                        jint jEndPointIn, jint jEndPointOut) {
        LOGE("进入C++OPEN");
//        d2cc=new D2cc;
        ffd=open(DATA_PATH, O_WRONLY|O_CREAT, 0);
        d2cc.OpenDevice(jfdesc, jEndPointIn, jEndPointOut);
//        D2cc::getInstance()->OpenDevice(jfdesc, jEndPointIn, jEndPointOut);
        //        LOGI("文件描述符%d",fdesc);
    }
    JNIEXPORT void JNICALL
    Java_ir_bigandsmall_hiddevice_D2ccDevice_CloseDevice(JNIEnv *env, jobject instance) {
        d2cc.CloseDevice();
    }
}
